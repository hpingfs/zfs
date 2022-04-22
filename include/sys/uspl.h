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

#ifndef	_SYS_USPL_H
#define	_SYS_USPL_H

// vmsystm.h
// FIXME(hping) should query kernel
extern unsigned long totalram_pages;
#define zfs_totalram_pages  totalram_pages

// sysmacros.h
#define is_system_labeled()     0

// cred.h
#define	KUID_TO_SUID(x)		(__kuid_val(x))
#define	KGID_TO_SGID(x)		(__kgid_val(x))
#define	SUID_TO_KUID(x)		(KUIDT_INIT(x))
#define	SGID_TO_KGID(x)		(KGIDT_INIT(x))
#define	KGIDP_TO_SGIDP(x)	(&(x)->val)

// FIXME(hping)
//#define current (0)

#define	mutex_owner(mp)		(((mp)->m_owner))
#define	mutex_owned(mp)		pthread_equal((mp)->m_owner, pthread_self())
#define	MUTEX_HELD(mp)		mutex_owned(mp)
#define	MUTEX_NOT_HELD(mp)	(!MUTEX_HELD(mp))

struct task_struct {};

int ddi_copyin(const void *from, void *to, size_t len, int flags);
int ddi_copyout(const void *from, void *to, size_t len, int flags);
int xcopyin(const void *from, void *to, size_t len);
int xcopyout(const void *from, void *to, size_t len);
int copyinstr(const void *from, void *to, size_t len, size_t *done);

boolean_t zfs_proc_is_caller(proc_t *t);

/*
 * Credentials
 */
extern void crhold(cred_t *cr);
extern void crfree(cred_t *cr);
extern uid_t crgetuid(cred_t *cr);
extern uid_t crgetruid(cred_t *cr);
extern gid_t crgetgid(cred_t *cr);
extern int crgetngroups(cred_t *cr);
extern gid_t *crgetgroups(cred_t *cr);

//#define FIGNORECASE     0x00080000
#define FKIOCTL         0x80000000
#define ED_CASE_CONFLICT    0x10

#ifdef HAVE_INODE_LOCK_SHARED
#define spl_inode_lock(ip)      inode_lock(ip)
#define spl_inode_unlock(ip)        inode_unlock(ip)
#define spl_inode_lock_shared(ip)   inode_lock_shared(ip)
#define spl_inode_unlock_shared(ip) inode_unlock_shared(ip)
#define spl_inode_trylock(ip)       inode_trylock(ip)
#define spl_inode_trylock_shared(ip)    inode_trylock_shared(ip)
#define spl_inode_is_locked(ip)     inode_is_locked(ip)
#define spl_inode_lock_nested(ip, s)    inode_lock_nested(ip, s)
#else
#define spl_inode_lock(ip)      mutex_lock(&(ip)->i_mutex)
#define spl_inode_unlock(ip)        mutex_unlock(&(ip)->i_mutex)
#define spl_inode_lock_shared(ip)   mutex_lock(&(ip)->i_mutex)
#define spl_inode_unlock_shared(ip) mutex_unlock(&(ip)->i_mutex)
#define spl_inode_trylock(ip)       mutex_trylock(&(ip)->i_mutex)
#define spl_inode_trylock_shared(ip)    mutex_trylock(&(ip)->i_mutex)
#define spl_inode_is_locked(ip)     mutex_is_locked(&(ip)->i_mutex)
#define spl_inode_lock_nested(ip, s)    mutex_lock_nested(&(ip)->i_mutex, s)
#endif

/*
 * 3.5 API change,
 * The clear_inode() function replaces end_writeback() and introduces an
 * ordering change regarding when the inode_sync_wait() occurs.  See the
 * configure check in config/kernel-clear-inode.m4 for full details.
 */
#if defined(HAVE_EVICT_INODE) && !defined(HAVE_CLEAR_INODE)
#define clear_inode(ip)     end_writeback(ip)
#endif /* HAVE_EVICT_INODE && !HAVE_CLEAR_INODE */

#endif	/* _SYS_USPL_H */
