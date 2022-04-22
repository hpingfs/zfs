/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2012, 2018 by Delphix. All rights reserved.
 * Copyright (c) 2016 Actifio, Inc. All rights reserved.
 */

#include <assert.h>
#include <fcntl.h>
#include <libgen.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libzutil.h>
#include <sys/crypto/icp.h>
#include <sys/processor.h>
#include <sys/rrwlock.h>
#include <sys/spa.h>
#include <sys/stat.h>
#include <sys/systeminfo.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/zfs_context.h>
#include <sys/zfs_onexit.h>
#include <sys/zfs_vfsops.h>
#include <sys/zfs_znode.h>
#include <sys/zfs_vnops.h>
#include <sys/zfs_ctldir.h>
#include <sys/zstd/zstd.h>
#include <sys/zvol.h>
#include <zfs_fletcher.h>
#include <zlib.h>

#include <math.h>

/*
 * Emulation of kernel services in userland.
 */

uint64_t physmem;
char hw_serial[HW_HOSTID_LEN];
struct utsname hw_utsname;

/* If set, all blocks read will be copied to the specified directory. */
char *vn_dumpdir = NULL;

/* this only exists to have its address taken */
struct proc p0;

/*
 * =========================================================================
 * threads
 * =========================================================================
 *
 * TS_STACK_MIN is dictated by the minimum allowed pthread stack size.  While
 * TS_STACK_MAX is somewhat arbitrary, it was selected to be large enough for
 * the expected stack depth while small enough to avoid exhausting address
 * space with high thread counts.
 */
#define	TS_STACK_MIN	MAX(PTHREAD_STACK_MIN, 32768)
#define	TS_STACK_MAX	(256 * 1024)

kthread_t *
zk_thread_create(void (*func)(void *), void *arg, size_t stksize, int state)
{
	pthread_attr_t attr;
	pthread_t tid;
	char *stkstr;
	int detachstate = PTHREAD_CREATE_DETACHED;

	VERIFY0(pthread_attr_init(&attr));

	if (state & TS_JOINABLE)
		detachstate = PTHREAD_CREATE_JOINABLE;

	VERIFY0(pthread_attr_setdetachstate(&attr, detachstate));

	/*
	 * We allow the default stack size in user space to be specified by
	 * setting the ZFS_STACK_SIZE environment variable.  This allows us
	 * the convenience of observing and debugging stack overruns in
	 * user space.  Explicitly specified stack sizes will be honored.
	 * The usage of ZFS_STACK_SIZE is discussed further in the
	 * ENVIRONMENT VARIABLES sections of the ztest(1) man page.
	 */
	if (stksize == 0) {
		stkstr = getenv("ZFS_STACK_SIZE");

		if (stkstr == NULL)
			stksize = TS_STACK_MAX;
		else
			stksize = MAX(atoi(stkstr), TS_STACK_MIN);
	}

	VERIFY3S(stksize, >, 0);
	stksize = P2ROUNDUP(MAX(stksize, TS_STACK_MIN), PAGESIZE);

	/*
	 * If this ever fails, it may be because the stack size is not a
	 * multiple of system page size.
	 */
	VERIFY0(pthread_attr_setstacksize(&attr, stksize));
	VERIFY0(pthread_attr_setguardsize(&attr, PAGESIZE));

	VERIFY0(pthread_create(&tid, &attr, (void *(*)(void *))func, arg));
	VERIFY0(pthread_attr_destroy(&attr));

	return ((void *)(uintptr_t)tid);
}

/*
 * =========================================================================
 * kstats
 * =========================================================================
 */
kstat_t *
kstat_create(const char *module, int instance, const char *name,
    const char *class, uchar_t type, ulong_t ndata, uchar_t ks_flag)
{
	(void) module, (void) instance, (void) name, (void) class, (void) type,
	    (void) ndata, (void) ks_flag;
	return (NULL);
}

void
kstat_install(kstat_t *ksp)
{
	(void) ksp;
}

void
kstat_delete(kstat_t *ksp)
{
	(void) ksp;
}

void
kstat_set_raw_ops(kstat_t *ksp,
    int (*headers)(char *buf, size_t size),
    int (*data)(char *buf, size_t size, void *data),
    void *(*addr)(kstat_t *ksp, loff_t index))
{
	(void) ksp, (void) headers, (void) data, (void) addr;
}

/*
 * =========================================================================
 * mutexes
 * =========================================================================
 */

void
mutex_init(kmutex_t *mp, char *name, int type, void *cookie)
{
	(void) name, (void) type, (void) cookie;
	VERIFY0(pthread_mutex_init(&mp->m_lock, NULL));
	memset(&mp->m_owner, 0, sizeof (pthread_t));
}

void
mutex_destroy(kmutex_t *mp)
{
	VERIFY0(pthread_mutex_destroy(&mp->m_lock));
}

void
mutex_enter(kmutex_t *mp)
{
	VERIFY0(pthread_mutex_lock(&mp->m_lock));
	mp->m_owner = pthread_self();
}

int
mutex_tryenter(kmutex_t *mp)
{
	int error = pthread_mutex_trylock(&mp->m_lock);
	if (error == 0) {
		mp->m_owner = pthread_self();
		return (1);
	} else {
		VERIFY3S(error, ==, EBUSY);
		return (0);
	}
}

void
mutex_exit(kmutex_t *mp)
{
	memset(&mp->m_owner, 0, sizeof (pthread_t));
	VERIFY0(pthread_mutex_unlock(&mp->m_lock));
}

/*
 * =========================================================================
 * rwlocks
 * =========================================================================
 */

void
rw_init(krwlock_t *rwlp, char *name, int type, void *arg)
{
	(void) name, (void) type, (void) arg;
	VERIFY0(pthread_rwlock_init(&rwlp->rw_lock, NULL));
	rwlp->rw_readers = 0;
	rwlp->rw_owner = 0;
}

void
rw_destroy(krwlock_t *rwlp)
{
	VERIFY0(pthread_rwlock_destroy(&rwlp->rw_lock));
}

void
rw_enter(krwlock_t *rwlp, krw_t rw)
{
	if (rw == RW_READER) {
		VERIFY0(pthread_rwlock_rdlock(&rwlp->rw_lock));
		atomic_inc_uint(&rwlp->rw_readers);
	} else {
		VERIFY0(pthread_rwlock_wrlock(&rwlp->rw_lock));
		rwlp->rw_owner = pthread_self();
	}
}

void
rw_exit(krwlock_t *rwlp)
{
	if (RW_READ_HELD(rwlp))
		atomic_dec_uint(&rwlp->rw_readers);
	else
		rwlp->rw_owner = 0;

	VERIFY0(pthread_rwlock_unlock(&rwlp->rw_lock));
}

int
rw_tryenter(krwlock_t *rwlp, krw_t rw)
{
	int error;

	if (rw == RW_READER)
		error = pthread_rwlock_tryrdlock(&rwlp->rw_lock);
	else
		error = pthread_rwlock_trywrlock(&rwlp->rw_lock);

	if (error == 0) {
		if (rw == RW_READER)
			atomic_inc_uint(&rwlp->rw_readers);
		else
			rwlp->rw_owner = pthread_self();

		return (1);
	}

	VERIFY3S(error, ==, EBUSY);

	return (0);
}

uint32_t
zone_get_hostid(void *zonep)
{
	/*
	 * We're emulating the system's hostid in userland.
	 */
	(void) zonep;
	return (strtoul(hw_serial, NULL, 10));
}

int
rw_tryupgrade(krwlock_t *rwlp)
{
	(void) rwlp;
	return (0);
}

/*
 * =========================================================================
 * condition variables
 * =========================================================================
 */

void
cv_init(kcondvar_t *cv, char *name, int type, void *arg)
{
	(void) name, (void) type, (void) arg;
	VERIFY0(pthread_cond_init(cv, NULL));
}

void
cv_destroy(kcondvar_t *cv)
{
	VERIFY0(pthread_cond_destroy(cv));
}

void
cv_wait(kcondvar_t *cv, kmutex_t *mp)
{
	memset(&mp->m_owner, 0, sizeof (pthread_t));
	VERIFY0(pthread_cond_wait(cv, &mp->m_lock));
	mp->m_owner = pthread_self();
}

int
cv_wait_sig(kcondvar_t *cv, kmutex_t *mp)
{
	cv_wait(cv, mp);
	return (1);
}

int
cv_timedwait(kcondvar_t *cv, kmutex_t *mp, clock_t abstime)
{
	int error;
	struct timeval tv;
	struct timespec ts;
	clock_t delta;

	delta = abstime - ddi_get_lbolt();
	if (delta <= 0)
		return (-1);

	VERIFY(gettimeofday(&tv, NULL) == 0);

	ts.tv_sec = tv.tv_sec + delta / hz;
	ts.tv_nsec = tv.tv_usec * NSEC_PER_USEC + (delta % hz) * (NANOSEC / hz);
	if (ts.tv_nsec >= NANOSEC) {
		ts.tv_sec++;
		ts.tv_nsec -= NANOSEC;
	}

	memset(&mp->m_owner, 0, sizeof (pthread_t));
	error = pthread_cond_timedwait(cv, &mp->m_lock, &ts);
	mp->m_owner = pthread_self();

	if (error == ETIMEDOUT)
		return (-1);

	VERIFY0(error);

	return (1);
}

int
cv_timedwait_hires(kcondvar_t *cv, kmutex_t *mp, hrtime_t tim, hrtime_t res,
    int flag)
{
	(void) res;
	int error;
	struct timeval tv;
	struct timespec ts;
	hrtime_t delta;

	ASSERT(flag == 0 || flag == CALLOUT_FLAG_ABSOLUTE);

	delta = tim;
	if (flag & CALLOUT_FLAG_ABSOLUTE)
		delta -= gethrtime();

	if (delta <= 0)
		return (-1);

	VERIFY0(gettimeofday(&tv, NULL));

	ts.tv_sec = tv.tv_sec + delta / NANOSEC;
	ts.tv_nsec = tv.tv_usec * NSEC_PER_USEC + (delta % NANOSEC);
	if (ts.tv_nsec >= NANOSEC) {
		ts.tv_sec++;
		ts.tv_nsec -= NANOSEC;
	}

	memset(&mp->m_owner, 0, sizeof (pthread_t));
	error = pthread_cond_timedwait(cv, &mp->m_lock, &ts);
	mp->m_owner = pthread_self();

	if (error == ETIMEDOUT)
		return (-1);

	VERIFY0(error);

	return (1);
}

void
cv_signal(kcondvar_t *cv)
{
	VERIFY0(pthread_cond_signal(cv));
}

void
cv_broadcast(kcondvar_t *cv)
{
	VERIFY0(pthread_cond_broadcast(cv));
}

/*
 * =========================================================================
 * procfs list
 * =========================================================================
 */

void
seq_printf(struct seq_file *m, const char *fmt, ...)
{
	(void) m, (void) fmt;
}

void
procfs_list_install(const char *module,
    const char *submodule,
    const char *name,
    mode_t mode,
    procfs_list_t *procfs_list,
    int (*show)(struct seq_file *f, void *p),
    int (*show_header)(struct seq_file *f),
    int (*clear)(procfs_list_t *procfs_list),
    size_t procfs_list_node_off)
{
	(void) module, (void) submodule, (void) name, (void) mode, (void) show,
	    (void) show_header, (void) clear;
	mutex_init(&procfs_list->pl_lock, NULL, MUTEX_DEFAULT, NULL);
	list_create(&procfs_list->pl_list,
	    procfs_list_node_off + sizeof (procfs_list_node_t),
	    procfs_list_node_off + offsetof(procfs_list_node_t, pln_link));
	procfs_list->pl_next_id = 1;
	procfs_list->pl_node_offset = procfs_list_node_off;
}

void
procfs_list_uninstall(procfs_list_t *procfs_list)
{
	(void) procfs_list;
}

void
procfs_list_destroy(procfs_list_t *procfs_list)
{
	ASSERT(list_is_empty(&procfs_list->pl_list));
	list_destroy(&procfs_list->pl_list);
	mutex_destroy(&procfs_list->pl_lock);
}

#define	NODE_ID(procfs_list, obj) \
		(((procfs_list_node_t *)(((char *)obj) + \
		(procfs_list)->pl_node_offset))->pln_id)

void
procfs_list_add(procfs_list_t *procfs_list, void *p)
{
	ASSERT(MUTEX_HELD(&procfs_list->pl_lock));
	NODE_ID(procfs_list, p) = procfs_list->pl_next_id++;
	list_insert_tail(&procfs_list->pl_list, p);
}

/*
 * =========================================================================
 * vnode operations
 * =========================================================================
 */

/*
 * =========================================================================
 * Figure out which debugging statements to print
 * =========================================================================
 */

static char *dprintf_string;
static int dprintf_print_all;

int
dprintf_find_string(const char *string)
{
	char *tmp_str = dprintf_string;
	int len = strlen(string);

	/*
	 * Find out if this is a string we want to print.
	 * String format: file1.c,function_name1,file2.c,file3.c
	 */

	while (tmp_str != NULL) {
		if (strncmp(tmp_str, string, len) == 0 &&
		    (tmp_str[len] == ',' || tmp_str[len] == '\0'))
			return (1);
		tmp_str = strchr(tmp_str, ',');
		if (tmp_str != NULL)
			tmp_str++; /* Get rid of , */
	}
	return (0);
}

void
dprintf_setup(int *argc, char **argv)
{
	int i, j;

	/*
	 * Debugging can be specified two ways: by setting the
	 * environment variable ZFS_DEBUG, or by including a
	 * "debug=..."  argument on the command line.  The command
	 * line setting overrides the environment variable.
	 */

	for (i = 1; i < *argc; i++) {
		int len = strlen("debug=");
		/* First look for a command line argument */
		if (strncmp("debug=", argv[i], len) == 0) {
			dprintf_string = argv[i] + len;
			/* Remove from args */
			for (j = i; j < *argc; j++)
				argv[j] = argv[j+1];
			argv[j] = NULL;
			(*argc)--;
		}
	}

	if (dprintf_string == NULL) {
		/* Look for ZFS_DEBUG environment variable */
		dprintf_string = getenv("ZFS_DEBUG");
	}

	/*
	 * Are we just turning on all debugging?
	 */
	if (dprintf_find_string("on"))
		dprintf_print_all = 1;

	if (dprintf_string != NULL)
		zfs_flags |= ZFS_DEBUG_DPRINTF;
}

/*
 * =========================================================================
 * debug printfs
 * =========================================================================
 */
void
__dprintf(boolean_t dprint, const char *file, const char *func,
    int line, const char *fmt, ...)
{
	/* Get rid of annoying "../common/" prefix to filename. */
	const char *newfile = zfs_basename(file);

	va_list adx;
	if (dprint) {
		/* dprintf messages are printed immediately */

		if (!dprintf_print_all &&
		    !dprintf_find_string(newfile) &&
		    !dprintf_find_string(func))
			return;

		/* Print out just the function name if requested */
		flockfile(stdout);
		if (dprintf_find_string("pid"))
			(void) printf("%d ", getpid());
		if (dprintf_find_string("tid"))
			(void) printf("%ju ",
			    (uintmax_t)(uintptr_t)pthread_self());
		if (dprintf_find_string("cpu"))
			(void) printf("%u ", getcpuid());
		if (dprintf_find_string("time"))
			(void) printf("%llu ", gethrtime());
		if (dprintf_find_string("long"))
			(void) printf("%s, line %d: ", newfile, line);
		(void) fprintf(stderr, "dprintf: %s: ", func);
		va_start(adx, fmt);
		(void) vfprintf(stderr, fmt, adx);
		va_end(adx);
		(void) fprintf(stderr, "\n");
		funlockfile(stdout);
	} else {
		/* zfs_dbgmsg is logged for dumping later */
		size_t size;
		char *buf;
		int i;

		size = 1024;
		buf = umem_alloc(size, UMEM_NOFAIL);
		i = snprintf(buf, size, "%s:%d:%s(): ", newfile, line, func);

		if (i < size) {
			va_start(adx, fmt);
			(void) vsnprintf(buf + i, size - i, fmt, adx);
			va_end(adx);
		}

		__zfs_dbgmsg(buf);

		umem_free(buf, size);
	}
}

/*
 * =========================================================================
 * cmn_err() and panic()
 * =========================================================================
 */
static char ce_prefix[CE_IGNORE][10] = { "", "NOTICE: ", "WARNING: ", "" };
static char ce_suffix[CE_IGNORE][2] = { "", "\n", "\n", "" };

void
vpanic(const char *fmt, va_list adx)
{
	(void) fprintf(stderr, "error: ");
	(void) vfprintf(stderr, fmt, adx);
	(void) fprintf(stderr, "\n");

	abort();	/* think of it as a "user-level crash dump" */
}

void
panic(const char *fmt, ...)
{
	va_list adx;

	va_start(adx, fmt);
	vpanic(fmt, adx);
	va_end(adx);
}

void
vcmn_err(int ce, const char *fmt, va_list adx)
{
	if (ce == CE_PANIC)
		vpanic(fmt, adx);
	if (ce != CE_NOTE) {	/* suppress noise in userland stress testing */
		(void) fprintf(stderr, "%s", ce_prefix[ce]);
		(void) vfprintf(stderr, fmt, adx);
		(void) fprintf(stderr, "%s", ce_suffix[ce]);
	}
}

void
cmn_err(int ce, const char *fmt, ...)
{
	va_list adx;

	va_start(adx, fmt);
	vcmn_err(ce, fmt, adx);
	va_end(adx);
}

/*
 * =========================================================================
 * misc routines
 * =========================================================================
 */

void
delay(clock_t ticks)
{
	(void) poll(0, 0, ticks * (1000 / hz));
}

/*
 * Find highest one bit set.
 * Returns bit number + 1 of highest bit that is set, otherwise returns 0.
 * The __builtin_clzll() function is supported by both GCC and Clang.
 */
int
highbit64(uint64_t i)
{
	if (i == 0)
	return (0);

	return (NBBY * sizeof (uint64_t) - __builtin_clzll(i));
}

/*
 * Find lowest one bit set.
 * Returns bit number + 1 of lowest bit that is set, otherwise returns 0.
 * The __builtin_ffsll() function is supported by both GCC and Clang.
 */
int
lowbit64(uint64_t i)
{
	if (i == 0)
		return (0);

	return (__builtin_ffsll(i));
}

const char *random_path = "/dev/random";
const char *urandom_path = "/dev/urandom";
static int random_fd = -1, urandom_fd = -1;

void
random_init(void)
{
	VERIFY((random_fd = open(random_path, O_RDONLY | O_CLOEXEC)) != -1);
	VERIFY((urandom_fd = open(urandom_path, O_RDONLY | O_CLOEXEC)) != -1);
}

void
random_fini(void)
{
	close(random_fd);
	close(urandom_fd);

	random_fd = -1;
	urandom_fd = -1;
}

static int
random_get_bytes_common(uint8_t *ptr, size_t len, int fd)
{
	size_t resid = len;
	ssize_t bytes;

	ASSERT(fd != -1);

	while (resid != 0) {
		bytes = read(fd, ptr, resid);
		ASSERT3S(bytes, >=, 0);
		ptr += bytes;
		resid -= bytes;
	}

	return (0);
}

int
random_get_bytes(uint8_t *ptr, size_t len)
{
	return (random_get_bytes_common(ptr, len, random_fd));
}

int
random_get_pseudo_bytes(uint8_t *ptr, size_t len)
{
	return (random_get_bytes_common(ptr, len, urandom_fd));
}

int
ddi_strtoul(const char *hw_serial, char **nptr, int base, unsigned long *result)
{
	(void) nptr;
	char *end;

	*result = strtoul(hw_serial, &end, base);
	if (*result == 0)
		return (errno);
	return (0);
}

int
ddi_strtoull(const char *str, char **nptr, int base, u_longlong_t *result)
{
	(void) nptr;
	char *end;

	*result = strtoull(str, &end, base);
	if (*result == 0)
		return (errno);
	return (0);
}

utsname_t *
utsname(void)
{
	return (&hw_utsname);
}

/*
 * =========================================================================
 * kernel emulation setup & teardown
 * =========================================================================
 */
static int
umem_out_of_memory(void)
{
	char errmsg[] = "out of memory -- generating core dump\n";

	(void) fprintf(stderr, "%s", errmsg);
	abort();
	return (0);
}

void
kernel_init(int mode)
{
	extern uint_t rrw_tsd_key;

	umem_nofail_callback(umem_out_of_memory);

	physmem = sysconf(_SC_PHYS_PAGES);

	dprintf("physmem = %llu pages (%.2f GB)\n", (u_longlong_t)physmem,
	    (double)physmem * sysconf(_SC_PAGE_SIZE) / (1ULL << 30));

	(void) snprintf(hw_serial, sizeof (hw_serial), "%ld",
	    (mode & SPA_MODE_WRITE) ? get_system_hostid() : 0);

	random_init();

	VERIFY0(uname(&hw_utsname));

	system_taskq_init();
	icp_init();

	zstd_init();

	spa_init((spa_mode_t)mode);

	fletcher_4_init();

	tsd_create(&rrw_tsd_key, rrw_tsd_destroy);
}

void
kernel_fini(void)
{
	fletcher_4_fini();
	spa_fini();

	zstd_fini();

	icp_fini();
	system_taskq_fini();

	random_fini();
}

//int
//zfs_secpolicy_snapshot_perms(const char *name, cred_t *cr)
//{
//	(void) name, (void) cr;
//	return (0);
//}
//
//int
//zfs_secpolicy_rename_perms(const char *from, const char *to, cred_t *cr)
//{
//	(void) from, (void) to, (void) cr;
//	return (0);
//}
//
//int
//zfs_secpolicy_destroy_perms(const char *name, cred_t *cr)
//{
//	(void) name, (void) cr;
//	return (0);
//}
//
//int
//secpolicy_zfs(const cred_t *cr)
//{
//	(void) cr;
//	return (0);
//}
//
//int
//secpolicy_zfs_proc(const cred_t *cr, proc_t *proc)
//{
//	(void) cr, (void) proc;
//	return (0);
//}

ksiddomain_t *
ksid_lookupdomain(const char *dom)
{
	ksiddomain_t *kd;

	kd = umem_zalloc(sizeof (ksiddomain_t), UMEM_NOFAIL);
	kd->kd_name = spa_strdup(dom);
	return (kd);
}

void
ksiddomain_rele(ksiddomain_t *ksid)
{
	spa_strfree(ksid->kd_name);
	umem_free(ksid, sizeof (ksiddomain_t));
}

char *
kmem_vasprintf(const char *fmt, va_list adx)
{
	char *buf = NULL;
	va_list adx_copy;

	va_copy(adx_copy, adx);
	VERIFY(vasprintf(&buf, fmt, adx_copy) != -1);
	va_end(adx_copy);

	return (buf);
}

char *
kmem_asprintf(const char *fmt, ...)
{
	char *buf = NULL;
	va_list adx;

	va_start(adx, fmt);
	VERIFY(vasprintf(&buf, fmt, adx) != -1);
	va_end(adx);

	return (buf);
}

zfs_file_t *
zfs_onexit_fd_hold(int fd, minor_t *minorp)
{
	(void) fd;
	*minorp = 0;
	return (NULL);
}

void
zfs_onexit_fd_rele(zfs_file_t *fp)
{
	(void) fp;
}

int
zfs_onexit_add_cb(minor_t minor, void (*func)(void *), void *data,
    uint64_t *action_handle)
{
	(void) minor, (void) func, (void) data, (void) action_handle;
	return (0);
}

fstrans_cookie_t
spl_fstrans_mark(void)
{
	return ((fstrans_cookie_t)0);
}

void
spl_fstrans_unmark(fstrans_cookie_t cookie)
{
	(void) cookie;
}

int
__spl_pf_fstrans_check(void)
{
	return (0);
}

int
kmem_cache_reap_active(void)
{
	return (0);
}

//void *zvol_tag = "zvol_tag";

void
zvol_create_minor(const char *name)
{
	(void) name;
}

void
zvol_create_minors_recursive(const char *name)
{
	(void) name;
}

void
zvol_remove_minors(spa_t *spa, const char *name, boolean_t async)
{
	(void) spa, (void) name, (void) async;
}

void
zvol_rename_minors(spa_t *spa, const char *oldname, const char *newname,
    boolean_t async)
{
	(void) spa, (void) oldname, (void) newname, (void) async;
}

/*
 * Open file
 *
 * path - fully qualified path to file
 * flags - file attributes O_READ / O_WRITE / O_EXCL
 * fpp - pointer to return file pointer
 *
 * Returns 0 on success underlying error on failure.
 */
int
zfs_file_open(const char *path, int flags, int mode, zfs_file_t **fpp)
{
	int fd = -1;
	int dump_fd = -1;
	int err;
	int old_umask = 0;
	zfs_file_t *fp;
	struct stat64 st;

	if (!(flags & O_CREAT) && stat64(path, &st) == -1)
		return (errno);

	if (!(flags & O_CREAT) && S_ISBLK(st.st_mode))
		flags |= O_DIRECT;

	if (flags & O_CREAT)
		old_umask = umask(0);

	fd = open64(path, flags, mode);
	if (fd == -1)
		return (errno);

	if (flags & O_CREAT)
		(void) umask(old_umask);

	if (vn_dumpdir != NULL) {
		char *dumppath = umem_zalloc(MAXPATHLEN, UMEM_NOFAIL);
		const char *inpath = zfs_basename(path);

		(void) snprintf(dumppath, MAXPATHLEN,
		    "%s/%s", vn_dumpdir, inpath);
		dump_fd = open64(dumppath, O_CREAT | O_WRONLY, 0666);
		umem_free(dumppath, MAXPATHLEN);
		if (dump_fd == -1) {
			err = errno;
			close(fd);
			return (err);
		}
	} else {
		dump_fd = -1;
	}

	(void) fcntl(fd, F_SETFD, FD_CLOEXEC);

	fp = umem_zalloc(sizeof (zfs_file_t), UMEM_NOFAIL);
	fp->f_fd = fd;
	fp->f_dump_fd = dump_fd;
	*fpp = fp;

	return (0);
}

void
zfs_file_close(zfs_file_t *fp)
{
	close(fp->f_fd);
	if (fp->f_dump_fd != -1)
		close(fp->f_dump_fd);

	umem_free(fp, sizeof (zfs_file_t));
}

/*
 * Stateful write - use os internal file pointer to determine where to
 * write and update on successful completion.
 *
 * fp -  pointer to file (pipe, socket, etc) to write to
 * buf - buffer to write
 * count - # of bytes to write
 * resid -  pointer to count of unwritten bytes  (if short write)
 *
 * Returns 0 on success errno on failure.
 */
int
zfs_file_write(zfs_file_t *fp, const void *buf, size_t count, ssize_t *resid)
{
	ssize_t rc;

	rc = write(fp->f_fd, buf, count);
	if (rc < 0)
		return (errno);

	if (resid) {
		*resid = count - rc;
	} else if (rc != count) {
		return (EIO);
	}

	return (0);
}

/*
 * Stateless write - os internal file pointer is not updated.
 *
 * fp -  pointer to file (pipe, socket, etc) to write to
 * buf - buffer to write
 * count - # of bytes to write
 * off - file offset to write to (only valid for seekable types)
 * resid -  pointer to count of unwritten bytes
 *
 * Returns 0 on success errno on failure.
 */
int
zfs_file_pwrite(zfs_file_t *fp, const void *buf,
    size_t count, loff_t pos, ssize_t *resid)
{
	ssize_t rc, split, done;
	int sectors;

	/*
	 * To simulate partial disk writes, we split writes into two
	 * system calls so that the process can be killed in between.
	 * This is used by ztest to simulate realistic failure modes.
	 */
	sectors = count >> SPA_MINBLOCKSHIFT;
	split = (sectors > 0 ? rand() % sectors : 0) << SPA_MINBLOCKSHIFT;
	rc = pwrite64(fp->f_fd, buf, split, pos);
	if (rc != -1) {
		done = rc;
		rc = pwrite64(fp->f_fd, (char *)buf + split,
		    count - split, pos + split);
	}
#ifdef __linux__
	if (rc == -1 && errno == EINVAL) {
		/*
		 * Under Linux, this most likely means an alignment issue
		 * (memory or disk) due to O_DIRECT, so we abort() in order
		 * to catch the offender.
		 */
		abort();
	}
#endif

	if (rc < 0)
		return (errno);

	done += rc;

	if (resid) {
		*resid = count - done;
	} else if (done != count) {
		return (EIO);
	}

	return (0);
}

/*
 * Stateful read - use os internal file pointer to determine where to
 * read and update on successful completion.
 *
 * fp -  pointer to file (pipe, socket, etc) to read from
 * buf - buffer to write
 * count - # of bytes to read
 * resid -  pointer to count of unread bytes (if short read)
 *
 * Returns 0 on success errno on failure.
 */
int
zfs_file_read(zfs_file_t *fp, void *buf, size_t count, ssize_t *resid)
{
	int rc;

	rc = read(fp->f_fd, buf, count);
	if (rc < 0)
		return (errno);

	if (resid) {
		*resid = count - rc;
	} else if (rc != count) {
		return (EIO);
	}

	return (0);
}

/*
 * Stateless read - os internal file pointer is not updated.
 *
 * fp -  pointer to file (pipe, socket, etc) to read from
 * buf - buffer to write
 * count - # of bytes to write
 * off - file offset to read from (only valid for seekable types)
 * resid -  pointer to count of unwritten bytes (if short write)
 *
 * Returns 0 on success errno on failure.
 */
int
zfs_file_pread(zfs_file_t *fp, void *buf, size_t count, loff_t off,
    ssize_t *resid)
{
	ssize_t rc;

	rc = pread64(fp->f_fd, buf, count, off);
	if (rc < 0) {
#ifdef __linux__
		/*
		 * Under Linux, this most likely means an alignment issue
		 * (memory or disk) due to O_DIRECT, so we abort() in order to
		 * catch the offender.
		 */
		if (errno == EINVAL)
			abort();
#endif
		return (errno);
	}

	if (fp->f_dump_fd != -1) {
		int status;

		status = pwrite64(fp->f_dump_fd, buf, rc, off);
		ASSERT(status != -1);
	}

	if (resid) {
		*resid = count - rc;
	} else if (rc != count) {
		return (EIO);
	}

	return (0);
}

/*
 * lseek - set / get file pointer
 *
 * fp -  pointer to file (pipe, socket, etc) to read from
 * offp - value to seek to, returns current value plus passed offset
 * whence - see man pages for standard lseek whence values
 *
 * Returns 0 on success errno on failure (ESPIPE for non seekable types)
 */
int
zfs_file_seek(zfs_file_t *fp, loff_t *offp, int whence)
{
	loff_t rc;

	rc = lseek(fp->f_fd, *offp, whence);
	if (rc < 0)
		return (errno);

	*offp = rc;

	return (0);
}

/*
 * Get file attributes
 *
 * filp - file pointer
 * zfattr - pointer to file attr structure
 *
 * Currently only used for fetching size and file mode
 *
 * Returns 0 on success or error code of underlying getattr call on failure.
 */
int
zfs_file_getattr(zfs_file_t *fp, zfs_file_attr_t *zfattr)
{
	struct stat64 st;

	if (fstat64_blk(fp->f_fd, &st) == -1)
		return (errno);

	zfattr->zfa_size = st.st_size;
	zfattr->zfa_mode = st.st_mode;

	return (0);
}

/*
 * Sync file to disk
 *
 * filp - file pointer
 * flags - O_SYNC and or O_DSYNC
 *
 * Returns 0 on success or error code of underlying sync call on failure.
 */
int
zfs_file_fsync(zfs_file_t *fp, int flags)
{
	(void) flags;

	if (fsync(fp->f_fd) < 0)
		return (errno);

	return (0);
}

/*
 * fallocate - allocate or free space on disk
 *
 * fp - file pointer
 * mode (non-standard options for hole punching etc)
 * offset - offset to start allocating or freeing from
 * len - length to free / allocate
 *
 * OPTIONAL
 */
int
zfs_file_fallocate(zfs_file_t *fp, int mode, loff_t offset, loff_t len)
{
#ifdef __linux__
	return (fallocate(fp->f_fd, mode, offset, len));
#else
	(void) fp, (void) mode, (void) offset, (void) len;
	return (EOPNOTSUPP);
#endif
}

/*
 * Request current file pointer offset
 *
 * fp - pointer to file
 *
 * Returns current file offset.
 */
loff_t
zfs_file_off(zfs_file_t *fp)
{
	return (lseek(fp->f_fd, SEEK_CUR, 0));
}

/*
 * unlink file
 *
 * path - fully qualified file path
 *
 * Returns 0 on success.
 *
 * OPTIONAL
 */
int
zfs_file_unlink(const char *path)
{
	return (remove(path));
}

/*
 * Get reference to file pointer
 *
 * fd - input file descriptor
 *
 * Returns pointer to file struct or NULL.
 * Unsupported in user space.
 */
zfs_file_t *
zfs_file_get(int fd)
{
	(void) fd;
	abort();
	return (NULL);
}
/*
 * Drop reference to file pointer
 *
 * fp - pointer to file struct
 *
 * Unsupported in user space.
 */
void
zfs_file_put(zfs_file_t *fp)
{
	abort();
	(void) fp;
}

void
zfsvfs_update_fromname(const char *oldname, const char *newname)
{
	(void) oldname, (void) newname;
}

// hping start
struct cred global_cred = {};
struct cred *kcred = &global_cred;

struct file_system_type uzfs_fs_type = {};
const struct super_operations uzfs_super_operations = {};
const struct export_operations uzfs_export_operations = {};
const struct dentry_operations uzfs_dentry_operations = {};
const struct inode_operations uzfs_inode_operations = {};
const struct inode_operations uzfs_dir_inode_operations = {};
const struct inode_operations uzfs_symlink_inode_operations = {};
const struct inode_operations uzfs_special_inode_operations = {};
const struct file_operations uzfs_file_operations = {};
const struct file_operations uzfs_dir_file_operations = {};
const struct address_space_operations uzfs_address_space_operations = {};

// zfs_ctldir
//const struct file_operations uzfs_fops_root = {};
//const struct inode_operations uzfs_ops_root = {};
//const struct file_operations uzfs_fops_snapdir = {};
//const struct inode_operations uzfs_ops_snapdir = {};
//const struct file_operations uzfs_fops_shares = {};
//const struct inode_operations uzfs_ops_shares = {};
const struct file_operations simple_dir_operations = {};
const struct inode_operations simple_dir_inode_operations = {};

int zfs_umount(struct super_block *sb)
{
    printf("%s\n", __func__);
    return 0;
}

static void __iget(struct inode* inode)
{
    printf("%s: %ld\n", __func__, inode->i_ino);
    atomic_inc_32(&inode->i_count.counter);
}

struct inode *igrab(struct inode *inode)
{
    printf("%s: %ld\n", __func__, inode->i_ino);
    __iget(inode);
    return inode;
//    spin_lock(&inode->i_lock);
//    if (!(inode->i_state & (I_FREEING|I_WILL_FREE))) {
//        __iget(inode);
//        spin_unlock(&inode->i_lock);
//    } else {
//        spin_unlock(&inode->i_lock);
//        /*
//         * Handle the case where s_op->clear_inode is not been
//         * called yet, and somebody is calling igrab
//         * while the inode is getting freed.
//         */
//        inode = NULL;
//    }
//    return inode;
}

/**
 *  iput    - put an inode
 *  @inode: inode to put
 *
 *  Puts an inode, dropping its usage count. If the inode use count hits
 *  zero, the inode is then freed and may also be destroyed.
 *
 *  Consequently, iput() can sleep.
 */
void iput(struct inode *inode)
{
    // FIXME(hping)
    if (inode) {
        printf("%s: %ld\n", __func__, inode->i_ino);
        atomic_dec_32(&inode->i_count.counter);
        if (atomic_read(&inode->i_count) == 0) {
            zfs_inactive(inode);
            destroy_inode(inode);
        }
    }
}


/**
 * drop_nlink - directly drop an inode's link count
 * @inode: inode
 *
 * This is a low-level filesystem helper to replace any
 * direct filesystem manipulation of i_nlink.  In cases
 * where we are attempting to track writes to the
 * filesystem, a decrement to zero means an imminent
 * write when the file is truncated and actually unlinked
 * on the filesystem.
 */
void drop_nlink(struct inode *inode)
{
    printf("%s: %ld\n", __func__, inode->i_ino);
    WARN_ON(inode->i_nlink == 0);
    inode->__i_nlink--;
//    if (!inode->i_nlink)
//        atomic_long_inc(&inode->i_sb->s_remove_count);
}

/**
 * clear_nlink - directly zero an inode's link count
 * @inode: inode
 *
 * This is a low-level filesystem helper to replace any
 * direct filesystem manipulation of i_nlink.  See
 * drop_nlink() for why we care about i_nlink hitting zero.
 */
void clear_nlink(struct inode *inode)
{
    printf("%s: %ld\n", __func__, inode->i_ino);
    if (inode->i_nlink) {
        inode->__i_nlink = 0;
//        atomic_long_inc(&inode->i_sb->s_remove_count);
    }
}

/**
 * set_nlink - directly set an inode's link count
 * @inode: inode
 * @nlink: new nlink (should be non-zero)
 *
 * This is a low-level filesystem helper to replace any
 * direct filesystem manipulation of i_nlink.
 */
void set_nlink(struct inode *inode, unsigned int nlink)
{
    printf("%s: %ld\n", __func__, inode->i_ino);
    if (!nlink) {
        clear_nlink(inode);
    } else {
        /* Yes, some filesystems do change nlink from zero to one */
//        if (inode->i_nlink == 0)
//            atomic_long_dec(&inode->i_sb->s_remove_count);

        inode->__i_nlink = nlink;
    }
}

/**
 * inc_nlink - directly increment an inode's link count
 * @inode: inode
 *
 * This is a low-level filesystem helper to replace any
 * direct filesystem manipulation of i_nlink.  Currently,
 * it is only here for parity with dec_nlink().
 */
void inc_nlink(struct inode *inode)
{
    printf("%s: %ld\n", __func__, inode->i_ino);
    if (unlikely(inode->i_nlink == 0)) {
//        WARN_ON(!(inode->i_state & I_LINKABLE));
//        atomic_long_dec(&inode->i_sb->s_remove_count);
    }

    inode->__i_nlink++;
}

struct inode *new_inode(struct super_block *sb) {
    struct inode *ip;

    VERIFY3S(zfs_inode_alloc(sb, &ip), ==, 0);
    inode_set_iversion(ip, 1);

    ip->i_sb = sb;
    ip->i_mapping = &ip->i_data;

    pthread_spin_init(&ip->i_lock, PTHREAD_PROCESS_PRIVATE);

    printf("%s: %ld\n", __func__, ip->i_ino);
    return (ip);
}

void destroy_inode(struct inode* inode) {
    printf("%s: %ld\n", __func__, inode->i_ino);
    if (inode) {
        if (inode->i_lock)
            pthread_spin_destroy(&inode->i_lock);
        zfs_inode_destroy(inode);
    }
}

/*
 * These are initializations that only need to be done
 * once, because the fields are idempotent across use
 * of the inode, so let the slab aware of that.
 */
void inode_init_once(struct inode *inode)
{
    printf("%s: %ld\n", __func__, inode->i_ino);
    memset(inode, 0, sizeof(*inode));
//    INIT_HLIST_NODE(&inode->i_hash);
//    INIT_LIST_HEAD(&inode->i_devices);
//    INIT_LIST_HEAD(&inode->i_io_list);
//    INIT_LIST_HEAD(&inode->i_lru);
//    address_space_init_once(&inode->i_data);
//    i_size_ordered_init(inode);
}

void
inode_set_iversion(struct inode *ip, uint64_t val)
{
    ip->i_version = val;
}

/**
 * write_inode_now  -   write an inode to disk
 * @inode: inode to write to disk
 * @sync: whether the write should be synchronous or not
 *
 * This function commits an inode to disk immediately if it is dirty. This is
 * primarily needed by knfsd.
 *
 * The caller must either have a ref on the inode or must have set I_WILL_FREE.
 */
int write_inode_now(struct inode *inode, int sync)
{
    printf("%s: %ld\n", __func__, inode->i_ino);
    return 0;
//    struct bdi_writeback *wb = &inode_to_bdi(inode)->wb;
//    struct writeback_control wbc = {
//        .nr_to_write = LONG_MAX,
//        .sync_mode = sync ? WB_SYNC_ALL : WB_SYNC_NONE,
//        .range_start = 0,
//        .range_end = LLONG_MAX,
//    };
//
//    if (!mapping_cap_writeback_dirty(inode->i_mapping))
//        wbc.nr_to_write = 0;
//
//    might_sleep();
//    return writeback_single_inode(inode, wb, &wbc);
}

/**
 *  __remove_inode_hash - remove an inode from the hash
 *  @inode: inode to unhash
 *
 *  Remove an inode from the superblock.
 */
void remove_inode_hash(struct inode *inode)
{
    printf("%s: %ld\n", __func__, inode->i_ino);
//    spin_lock(&inode_hash_lock);
//    spin_lock(&inode->i_lock);
//    hlist_del_init(&inode->i_hash);
//    spin_unlock(&inode->i_lock);
//    spin_unlock(&inode_hash_lock);
}

/**
 * inode_owner_or_capable - check current task permissions to inode
 * @inode: inode being checked
 *
 * Return true if current either has CAP_FOWNER in a namespace with the
 * inode owner uid mapped, or owns the file.
 */
boolean_t inode_owner_or_capable(const struct inode *inode)
{
    printf("%s: %ld\n", __func__, inode->i_ino);
//    struct user_namespace *ns;
//
//    if (uid_eq(current_fsuid(), inode->i_uid))
//        return true;
//
//    ns = current_user_ns();
//    if (ns_capable(ns, CAP_FOWNER) && kuid_has_mapping(ns, inode->i_uid))
//        return true;
    return B_FALSE;
}

/**
 * unlock_new_inode - clear the I_NEW state and wake up any waiters
 * @inode:	new inode to unlock
 *
 * Called when the inode is fully initialised to clear the new state of the
 * inode and wake up anyone waiting for the inode to finish initialisation.
 */
void unlock_new_inode(struct inode *inode)
{
    printf("%s: %ld\n", __func__, inode->i_ino);
//	lockdep_annotate_inode_mutex_key(inode);
//	spin_lock(&inode->i_lock);
//	WARN_ON(!(inode->i_state & I_NEW));
//	inode->i_state &= ~I_NEW & ~I_CREATING;
//	smp_mb();
//	wake_up_bit(&inode->i_state, __I_NEW);
//	spin_unlock(&inode->i_lock);
}

int insert_inode_locked(struct inode *inode)
{
    printf("%s: %ld\n", __func__, inode->i_ino);
    __iget(inode);
    return 0;
}

/*
 * NOTE: unlike i_size_read(), i_size_write() does need locking around it
 * (normally i_mutex), otherwise on 32bit/SMP an update of i_size_seqcount
 * can be lost, resulting in subsequent i_size_read() calls spinning forever.
 */
void i_size_write(struct inode *inode, loff_t i_size)
{
    printf("%s: %ld\n", __func__, inode->i_ino);
	inode->i_size = i_size;
}

/*
 * inode_set_flags - atomically set some inode flags
 *
 * Note: the caller should be holding i_mutex, or else be sure that
 * they have exclusive access to the inode structure (i.e., while the
 * inode is being instantiated).  The reason for the cmpxchg() loop
 * --- which wouldn't be necessary if all code paths which modify
 * i_flags actually followed this rule, is that there is at least one
 * code path which doesn't today so we use cmpxchg() out of an abundance
 * of caution.
 *
 * In the long run, i_mutex is overkill, and we should probably look
 * at using the i_lock spinlock to protect i_flags, and then make sure
 * it is so documented in include/linux/fs.h and that all code follows
 * the locking convention!!
 */
void inode_set_flags(struct inode *inode, unsigned int flags, unsigned int mask)
{
    printf("%s: %ld\n", __func__, inode->i_ino);
//	unsigned int old_flags, new_flags;
//
//	WARN_ON_ONCE(flags & ~mask);
//	do {
//		old_flags = ACCESS_ONCE(inode->i_flags);
//		new_flags = (old_flags & ~mask) | flags;
//	} while (unlikely(cmpxchg(&inode->i_flags, old_flags,
//				  new_flags) != old_flags));
}

void mark_inode_dirty(struct inode *inode)
{
    printf("%s: %ld\n", __func__, inode->i_ino);
}

struct dentry *d_make_root(struct inode *root_inode) {
    printf("%s: %ld\n", __func__, root_inode->i_ino);
    return NULL;
}

void d_prune_aliases(struct inode *inode) {
    printf("%s: %ld\n", __func__, inode->i_ino);
}

/**
 * shrink_dcache_sb - shrink dcache for a superblock
 * @sb: superblock
 *
 * Shrink the dcache for the specified super block. This is used to free
 * the dcache before unmounting a file system.
 */
void shrink_dcache_sb(struct super_block *sb)
{
    printf("%s\n", __func__);
//    LIST_HEAD(tmp);
//
//    spin_lock(&dcache_lru_lock);
//    while (!list_empty(&sb->s_dentry_lru)) {
//        list_splice_init(&sb->s_dentry_lru, &tmp);
//        spin_unlock(&dcache_lru_lock);
//        shrink_dentry_list(&tmp);
//        spin_lock(&dcache_lru_lock);
//    }
//    spin_unlock(&dcache_lru_lock);
}

int uzfs_dir_emit(void *ctx, const char *name, int namelen, loff_t off, uint64_t ino, unsigned type)
{
    printf("\t%s\tobjnum: %ld\n", name, ino);
    return 0;
}
//
//boolean_t zpl_dir_emit(zpl_dir_context_t *ctx, const char *name, int namelen, uint64_t ino, unsigned type)
//{
//    printf("\t%s\tobjnum: %ld\n", name, ino);
//    return 1;
//}
//
//boolean_t zpl_dir_emit_dot(struct file *file, zpl_dir_context_t *ctx)
//{
//    printf("\t.\tobjnum: %ld\n", file->f_inode->i_ino);
//    return 1;
//}
//
//boolean_t zpl_dir_emit_dotdot(struct file *file, zpl_dir_context_t *ctx)
//{
//    printf("\t..\tobjnum: %ld\n", file->f_inode->i_ino);
//    return 1;
//}
//
//boolean_t zpl_dir_emit_dots(struct file *file, zpl_dir_context_t *ctx)
//{
//    if (ctx->pos == 0) {
//        if (!zpl_dir_emit_dot(file, ctx))
//            return (B_FALSE);
//        ctx->pos = 1;
//    }
//    if (ctx->pos == 1) {
//        if (!zpl_dir_emit_dotdot(file, ctx))
//            return (B_FALSE);
//        ctx->pos = 2;
//    }
//    return (B_TRUE);
//}
//

void update_pages(znode_t *zp, int64_t start, int len, objset_t *os)
{
    printf("%s: %ld\n", __func__, ZTOI(zp)->i_ino);
}

int mappedread(znode_t *zp, int nbytes, zfs_uio_t *uio) {
    printf("%s: %ld\n", __func__, ZTOI(zp)->i_ino);
    return 0;
}

uid_t zfs_uid_read(struct inode *inode)
{
    printf("%s: %ld\n", __func__, inode->i_ino);
    return 0;
//	return (zfs_uid_read_impl(ip));
}

gid_t zfs_gid_read(struct inode *inode)
{
    printf("%s: %ld\n", __func__, inode->i_ino);
    return 0;
//	return (zfs_gid_read_impl(ip));
}

void zfs_uid_write(struct inode *inode, uid_t uid)
{
    printf("%s: %ld\n", __func__, inode->i_ino);
	inode->i_uid = make_kuid(kcred->user_ns, uid);
}

void zfs_gid_write(struct inode *inode, gid_t gid)
{
    printf("%s: %ld\n", __func__, inode->i_ino);
	inode->i_gid = make_kgid(kcred->user_ns, gid);
}

/**
 * truncate_setsize - update inode and pagecache for a new file size
 * @inode: inode
 * @newsize: new file size
 *
 * truncate_setsize updates i_size and performs pagecache truncation (if
 * necessary) to @newsize. It will be typically be called from the filesystem's
 * setattr function when ATTR_SIZE is passed in.
 *
 * Must be called with inode_mutex held and before all filesystem specific
 * block truncation has been performed.
 */
void truncate_setsize(struct inode *inode, loff_t newsize)
{
    printf("%s: %ld\n", __func__, inode->i_ino);
//	loff_t oldsize = inode->i_size;
//
//	i_size_write(inode, newsize);
//	if (newsize > oldsize)
//		pagecache_isize_extended(inode, oldsize, newsize);
//	truncate_pagecache(inode, newsize);
}

int register_filesystem(struct file_system_type * fs)
{
    printf("%s\n", __func__);
	return 0;
}

int unregister_filesystem(struct file_system_type * fs)
{
    printf("%s\n", __func__);
	return 0;
}

void deactivate_super(struct super_block *s) {
    printf("%s\n", __func__);
}

// huangping: common utility

int atomic_read(const atomic_t *v)
{
    return atomic_load_int(&v->counter);
}

/**
 * atomic_set - set atomic variable
 * @v: pointer of type atomic_t
 * @i: required value
 *
 * Atomically sets the value of @v to @i.
 */
void atomic_set(atomic_t *v, int i)
{
    atomic_store_int(&v->counter, i);
}


/**
 * __atomic_add_unless - add unless the number is already a given value
 * @v: pointer of type atomic_t
 * @a: the amount to add to v...
 * @u: ...unless v is equal to u.
 *
 * Atomically adds @a to @v, so long as @v was not already @u.
 * Returns the old value of @v.
 */
int atomic_add_unless(atomic_t *v, int a, int u)
{
	int c, old;
	c = atomic_read(v);
	for (;;) {
		if (unlikely(c == (u)))
			break;
		old = atomic_cas_32((volatile uint32_t*)(&v->counter), c, c + (a));
		if (likely(old == c))
			break;
		c = old;
	}
	return c;
}


/**
 * timespec_trunc - Truncate timespec to a granularity
 * @t: Timespec
 * @gran: Granularity in ns.
 *
 * Truncate a timespec to a granularity. gran must be smaller than a second.
 * Always rounds down.
 *
 * This function should be only used for timestamps returned by
 * current_kernel_time() or CURRENT_TIME, not with do_gettimeofday() because
 * it doesn't handle the better resolution of the latter.
 */
struct timespec timespec_trunc(struct timespec t, unsigned gran)
{
    printf("%s\n", __func__);
//    /*
//     * Division is pretty slow so avoid it for common cases.
//     * Currently current_kernel_time() never returns better than
//     * jiffies resolution. Exploit that.
//     */
//    if (gran <= jiffies_to_usecs(1) * 1000) {
//        /* nothing */
//    } else if (gran == 1000000000) {
//        t.tv_nsec = 0;
//    } else {
//        t.tv_nsec -= t.tv_nsec % gran;
//    }
    return t;
}

struct timespec current_kernel_time(void)
{
    printf("%s\n", __func__);
    struct timespec now = {0};
    return now;
//    struct timekeeper *tk = &timekeeper;
//    struct timespec64 now;
//    unsigned long seq;
//
//    do {
//        seq = read_seqcount_begin(&timekeeper_seq);
//
//        now = tk_xtime(tk);
//    } while (read_seqcount_retry(&timekeeper_seq, seq));
//
//    return timespec64_to_timespec(now);
}

#if !defined(HAVE_CURRENT_TIME)
struct timespec current_time(struct inode *inode)
{
    printf("%s: %ld\n", __func__, inode->i_ino);
    return (timespec_trunc(current_kernel_time(), inode->i_sb->s_time_gran));
}
#endif


/*
 * lhs < rhs:  return <0
 * lhs == rhs: return 0
 * lhs > rhs:  return >0
 */
int timespec_compare(const struct timespec *lhs, const struct timespec *rhs)
{
    printf("%s\n", __func__);
	if (lhs->tv_sec < rhs->tv_sec)
		return -1;
	if (lhs->tv_sec > rhs->tv_sec)
		return 1;
	return lhs->tv_nsec - rhs->tv_nsec;
}

int groupmember(gid_t gid, const cred_t *cr) {
    printf("%s\n", __func__);
    return 0;
}

/* Return the filesystem user id */
uid_t crgetfsuid(const cred_t *cr)
{
    printf("%s\n", __func__);
    return 0;
//	return (KUID_TO_SUID(cr->fsuid));
}

/* Return the filesystem group id */
gid_t crgetfsgid(const cred_t *cr)
{
    printf("%s\n", __func__);
    return 0;
//	return (KGID_TO_SGID(cr->fsgid));
}

/**
 * fls - find last set bit in word
 * @x: the word to search
 *
 * This is defined in a similar way as the libc and compiler builtin
 * ffs, but returns the position of the most significant set bit.
 *
 * fls(value) returns 0 if value is 0 or the position of the last
 * set bit if value is nonzero. The last (most significant) bit is
 * at position 32.
 */
int fls(int x)
{
    int r;

#ifdef CONFIG_X86_64
    /*
     * AMD64 says BSRL won't clobber the dest reg if x==0; Intel64 says the
     * dest reg is undefined if x==0, but their CPU architect says its
     * value is written to set it to the same as before, except that the
     * top 32 bits will be cleared.
     *
     * We cannot do this on 32 bits because at the very least some
     * 486 CPUs did not behave this way.
     */
    asm("bsrl %1,%0"
        : "=r" (r)
        : "rm" (x), "0" (-1));
#elif defined(CONFIG_X86_CMOV)
    asm("bsrl %1,%0\n\t"
        "cmovzl %2,%0"
        : "=&r" (r) : "rm" (x), "rm" (-1));
#else
    asm("bsrl %1,%0\n\t"
        "jnz 1f\n\t"
        "movl $-1,%0\n"
        "1:" : "=r" (r) : "rm" (x));
#endif
    return r + 1;
}

int ilog2(uint64_t n) { return (int)log2(n); }

/**
 * has_capability - Does a task have a capability in init_user_ns
 * @t: The task in question
 * @cap: The capability to be tested for
 *
 * Return true if the specified task has the given superior capability
 * currently in effect to the initial user namespace, false if not.
 *
 * Note that this does not set PF_SUPERPRIV on the task.
 */
boolean_t has_capability(proc_t *t, int cap)
{
    return B_FALSE;
//    return has_ns_capability(t, &init_user_ns, cap);
}

/**
 * capable - Determine if the current task has a superior capability in effect
 * @cap: The capability to be tested for
 *
 * Return true if the current task has the given superior capability currently
 * available for use, false if not.
 *
 * This sets PF_SUPERPRIV on the task if the capability is available on the
 * assumption that it's about to be used.
 */
boolean_t capable(int cap)
{
    return B_TRUE;
//    return ns_capable(&init_user_ns, cap);
}


long zfsdev_ioctl(unsigned cmd, unsigned long arg)
{
	uint_t vecnum;
	zfs_cmd_t *zc;
	int error;

	vecnum = cmd - ZFS_IOC_FIRST;

	zc = kmem_zalloc(sizeof (zfs_cmd_t), KM_SLEEP);

	if (ddi_copyin((void *)(uintptr_t)arg, zc, sizeof (zfs_cmd_t), 0)) {
		error = -SET_ERROR(EFAULT);
		goto out;
	}
	error = -zfsdev_ioctl_common(vecnum, zc, 0);
	ddi_copyout(zc, (void *)(uintptr_t)arg, sizeof (zfs_cmd_t), 0);
	if (error != 0)
		error = -SET_ERROR(EFAULT);
out:
	kmem_free(zc, sizeof (zfs_cmd_t));
	return (error);

}

void schedule() {
    printf("%s\n", __func__);
}

void init_special_inode(struct inode *inode, umode_t mode, dev_t dev)
{
    printf("%s\n", __func__);
}

void truncate_inode_pages_range(struct address_space *space, loff_t lstart, loff_t lend)
{
    printf("%s\n", __func__);
}

struct inode *ilookup(struct super_block *sb, unsigned long ino)
{
    printf("%s, ino: %ld\n", __func__, ino);
    return NULL;
}

struct dentry * d_obtain_alias(struct inode *inode)
{
    printf("%s\n", __func__);
    return NULL;
}

boolean_t d_mountpoint(struct dentry *dentry)
{
    printf("%s\n", __func__);
    return B_TRUE;
}

void dput(struct dentry *dentry)
{
    printf("%s\n", __func__);
}

int zfsctl_snapshot_unmount(const char *snapname, int flags)
{
    printf("%s, snapname: %s\n", __func__, snapname);
    return 0;
}

int kern_path(const char *name, unsigned int flags, struct path *path)
{
    printf("%s, name: %s\n", __func__, name);
    return 0;
//    struct nameidata nd;
//    int res = do_path_lookup(AT_FDCWD, name, flags, &nd);
//    if (!res)
//        *path = nd.path;
//    return res;
}

void zfs_zero_partial_page(znode_t *zp, uint64_t start, uint64_t len)
{
    // never be called
    printf("%s\n", __func__);
    ASSERT(0);
}

void path_put(const struct path *path)
{
    printf("%s\n", __func__);
    ASSERT(0);
}

int generic_file_open(struct inode * inode, struct file * filp)
{
    printf("%s\n", __func__);
    ASSERT(0);
    return 0;
}

struct dentry * d_splice_alias(struct inode *inode, struct dentry *dentry)
{
    printf("%s\n", __func__);
    ASSERT(0);
    return NULL;
}

void d_set_d_op(struct dentry *dentry, const struct dentry_operations *op)
{
    printf("%s\n", __func__);
    ASSERT(0);
}

void d_instantiate(struct dentry *dentry, struct inode *inode)
{
    printf("%s\n", __func__);
    ASSERT(0);
}

void generic_fillattr(struct inode *inode, struct linux_kstat *stat)
{
    printf("%s\n", __func__);
    ASSERT(0);
}

loff_t generic_file_llseek(struct file *file, loff_t offset, int whence)
{
    printf("%s\n", __func__);
    ASSERT(0);
    return 0;
}

ssize_t generic_read_dir(struct file * file, char *buf, size_t size, loff_t *off)
{
    printf("%s\n", __func__);
    ASSERT(0);
    return 0;
}

int zfsctl_snapshot_mount(struct path *path, int flags)
{
    printf("%s\n", __func__);
    ASSERT(0);
    return 0;
}

//void
//zpl_vap_init(vattr_t *vap, struct inode *dir, umode_t mode, cred_t *cr)
//{
//    vap->va_mask = ATTR_MODE;
//    vap->va_mode = mode;
//    vap->va_uid = crgetfsuid(cr);
//
//    if (dir && dir->i_mode & S_ISGID) {
//        vap->va_gid = KGID_TO_SGID(dir->i_gid);
//        if (S_ISDIR(mode))
//            vap->va_mode |= S_ISGID;
//    } else {
//        vap->va_gid = crgetfsgid(cr);
//    }
//}

ssize_t generic_getxattr(struct dentry *dentry, const char *name, void *buffer, size_t size)
{
    printf("%s\n", __func__);
    ASSERT(0);
    return 0;
}

ssize_t generic_listxattr(struct dentry *dentry, char *buffer, size_t buffer_size)
{
    printf("%s\n", __func__);
    ASSERT(0);
    return 0;
}

int generic_setxattr(struct dentry *dentry, const char *name, const void *value, size_t size, int flags)
{
    printf("%s\n", __func__);
    ASSERT(0);
    return 0;
}

int generic_removexattr(struct dentry *dentry, const char *name)
{
    printf("%s\n", __func__);
    ASSERT(0);
    return 0;
}

int generic_readlink(struct dentry *dentry, char *buffer, int buflen)
{
    printf("%s\n", __func__);
    ASSERT(0);
    return 0;
}

struct dentry *d_add_ci(struct dentry *dentry, struct inode *inode, struct qstr *name)
{
    printf("%s\n", __func__);
    ASSERT(0);
    return NULL;
}

int d_invalidate(struct dentry *dentry)
{
    printf("%s\n", __func__);
    ASSERT(0);
    return 0;
}

int inode_change_ok(const struct inode *inode, struct iattr *attr)
{
    printf("%s\n", __func__);
    ASSERT(0);
    return 0;
}

ssize_t do_sync_read(struct file *filp, char *buf, size_t len, loff_t *ppos)
{
    printf("%s\n", __func__);
    ASSERT(0);
    return 0;
}

ssize_t do_sync_write(struct file *filp, const char *buf, size_t len, loff_t *ppos)
{
    printf("%s\n", __func__);
    ASSERT(0);
    return 0;
}

int __set_page_dirty_nobuffers(struct page *page)
{
    printf("%s\n", __func__);
    ASSERT(0);
    return 0;
}

int filemap_write_and_wait_range(struct address_space *mapping, loff_t lstart, loff_t lend)
{
    printf("%s\n", __func__);
    ASSERT(0);
    return 0;
}

void touch_atime(struct path *path)
{
    printf("%s\n", __func__);
    ASSERT(0);
}

int generic_segment_checks(const struct iovec *iov, unsigned long *nr_segs, size_t *count, int access_flags)
{
    printf("%s\n", __func__);
    ASSERT(0);
    return 0;
}


int generic_write_checks(struct file *file, loff_t *pos, size_t *count, int isblk)
{
    printf("%s\n", __func__);
    ASSERT(0);
    return 0;
}


