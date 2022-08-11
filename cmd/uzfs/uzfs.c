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
 * Copyright (c) 2011 Lawrence Livermore National Security, LLC.
 */

#include <libintl.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <libzfs.h>
#include <locale.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/zfs_context.h>
#include <libuzfs.h>
#include <pthread.h>

libzfs_handle_t *g_zfs;
static int uzfs_fd_rand = -1;
static int uzfs_do_stat(int argc, char **argv);
static int uzfs_do_read(int argc, char **argv);
static int uzfs_do_write(int argc, char **argv);
static int uzfs_do_fsync(int argc, char **argv);
static int uzfs_do_setxattr(int argc, char **argv);
static int uzfs_do_getxattr(int argc, char **argv);
static int uzfs_do_mkdir(int argc, char **argv);
static int uzfs_do_create(int argc, char **argv);
static int uzfs_do_rm(int argc, char **argv);
static int uzfs_do_ls(int argc, char **argv);
static int uzfs_do_truncate(int argc, char **argv);
static int uzfs_do_fallocate(int argc, char **argv);
static int uzfs_do_mv(int argc, char **argv);

static int uzfs_perf(int argc, char **argv);

static int uzfs_olist(int argc, char **argv);
static int uzfs_ostat(int argc, char **argv);
static int uzfs_ocreate(int argc, char **argv);
static int uzfs_oclaim(int argc, char **argv);
static int uzfs_ocp(int argc, char **argv);
static int uzfs_orm(int argc, char **argv);
static int uzfs_oread(int argc, char **argv);
static int uzfs_owrite(int argc, char **argv);
static int uzfs_operf(int argc, char **argv);

typedef enum {
	HELP_STAT,
	HELP_READ,
	HELP_WRITE,
	HELP_FSYNC,
	HELP_SETXATTR,
	HELP_GETXATTR,
	HELP_MKDIR,
	HELP_CREATE,
	HELP_RM,
	HELP_LS,
	HELP_TRUNCATE,
	HELP_FALLOCATE,
	HELP_MV,
	HELP_PERF,
	HELP_OLIST,
	HELP_OSTAT,
	HELP_OCREATE,
	HELP_OCLAIM,
	HELP_OCP,
	HELP_ORM,
	HELP_OREAD,
	HELP_OWRITE,
	HELP_OPERF,
} uzfs_help_t;

typedef struct uzfs_command {
	const char	*name;
	int		(*func)(int argc, char **argv);
	uzfs_help_t	usage;
} uzfs_command_t;

/*
 * Master command table.  Each ZFS command has a name, associated function, and
 * usage message.  The usage messages need to be internationalized, so we have
 * to have a function to return the usage message based on a command index.
 *
 * These commands are organized according to how they are displayed in the usage
 * message.  An empty command (one with a NULL name) indicates an empty line in
 * the generic usage message.
 */
static uzfs_command_t command_table[] = {
	{ "stat",	uzfs_do_stat, 	HELP_STAT		},
	{ "read",	uzfs_do_read, 	HELP_READ		},
	{ "write",	uzfs_do_write, 	HELP_WRITE		},
	{ "fsync",	uzfs_do_fsync, 	HELP_FSYNC		},
	{ "setxattr",	uzfs_do_setxattr, 	HELP_SETXATTR		},
	{ "getxattr",	uzfs_do_getxattr, 	HELP_GETXATTR		},
	{ "mkdir",	uzfs_do_mkdir, 	HELP_MKDIR		},
	{ "create",	uzfs_do_create, 	HELP_CREATE		},
	{ "rm",	uzfs_do_rm, 	HELP_RM		},
	{ "ls",	uzfs_do_ls, 	HELP_LS		},
	{ "truncate",	uzfs_do_truncate, 	HELP_TRUNCATE		},
	{ "fallocate",	uzfs_do_fallocate, 	HELP_FALLOCATE		},
	{ "mv",	uzfs_do_mv, 	HELP_MV		},

	{ "perf",	uzfs_perf, 	HELP_PERF		},

    // following ops are object-based

	{ "olist",	uzfs_olist, 	HELP_OLIST		},
	{ "ostat",	uzfs_ostat, 	HELP_OSTAT		},
	{ "ocreate",	uzfs_ocreate, 	HELP_OCREATE		},
	{ "oclaim",	uzfs_oclaim, 	HELP_OCLAIM		},
	{ "ocp",	uzfs_ocp, 	HELP_OCP		},
	{ "orm",	uzfs_orm, 	HELP_ORM		},
	{ "oread",	uzfs_oread, 	HELP_OREAD		},
	{ "owrite",	uzfs_owrite, 	HELP_OWRITE		},
	{ "operf",	uzfs_operf, 	HELP_OPERF		},
};

#define	NCOMMAND	(sizeof (command_table) / sizeof (command_table[0]))

uzfs_command_t *current_command;

static const char *
get_usage(uzfs_help_t idx)
{
	switch (idx) {
	case HELP_STAT:
		return (gettext("\tstat ...\n"));
	case HELP_READ:
		return (gettext("\tread ...\n"));
	case HELP_WRITE:
		return (gettext("\twrite ...\n"));
	case HELP_FSYNC:
		return (gettext("\tfsync ...\n"));
	case HELP_SETXATTR:
		return (gettext("\tsetxattr ...\n"));
	case HELP_GETXATTR:
		return (gettext("\tgetxattr ...\n"));
	case HELP_MKDIR:
		return (gettext("\tmkdir ...\n"));
	case HELP_CREATE:
		return (gettext("\tcreate ...\n"));
	case HELP_RM:
		return (gettext("\trm ...\n"));
	case HELP_TRUNCATE:
		return (gettext("\ttruncate ...\n"));
	case HELP_FALLOCATE:
		return (gettext("\tfallocate ...\n"));
    case HELP_MV:
		return (gettext("\tmv ...\n"));
	case HELP_PERF:
		return (gettext("\tperf ...\n"));
	case HELP_OLIST:
		return (gettext("\tolist ...\n"));
	case HELP_OSTAT:
		return (gettext("\tostat ...\n"));
	case HELP_OCREATE:
		return (gettext("\tocreate ...\n"));
	case HELP_OCLAIM:
		return (gettext("\toclaim ...\n"));
	case HELP_OCP:
		return (gettext("\tocp ...\n"));
	case HELP_ORM:
		return (gettext("\torm ...\n"));
	case HELP_OREAD:
		return (gettext("\toread ...\n"));
	case HELP_OWRITE:
		return (gettext("\towrite ...\n"));
	case HELP_OPERF:
		return (gettext("\toperf ...\n"));
	default:
		__builtin_unreachable();
	}
}
/*
 * Display usage message.  If we're inside a command, display only the usage for
 * that command.  Otherwise, iterate over the entire command table and display
 * a complete usage message.
 */
static void
usage(boolean_t requested)
{
	int i;
	FILE *fp = requested ? stdout : stderr;

	if (current_command == NULL) {

		(void) fprintf(fp, gettext("usage: uzfs command args ...\n"));
		(void) fprintf(fp,
		    gettext("where 'command' is one of the following:\n\n"));

		for (i = 0; i < NCOMMAND; i++) {
			if (command_table[i].name == NULL)
				(void) fprintf(fp, "\n");
			else
				(void) fprintf(fp, "%s",
				    get_usage(command_table[i].usage));
		}

		(void) fprintf(fp, gettext("\nEach dataset is of the form: "
		    "pool/[dataset/]*dataset[@name]\n"));
	} else {
		(void) fprintf(fp, gettext("usage:\n"));
		(void) fprintf(fp, "%s", get_usage(current_command->usage));
	}

	/*
	 * See comments at end of main().
	 */
	if (getenv("ZFS_ABORT") != NULL) {
		(void) printf("dumping core by request\n");
		abort();
	}

	exit(requested ? 0 : 2);
}


static int
find_command_idx(char *command, int *idx)
{
	int i;

	for (i = 0; i < NCOMMAND; i++) {
		if (command_table[i].name == NULL)
			continue;

		if (strcmp(command, command_table[i].name) == 0) {
			*idx = i;
			return (0);
		}
	}
	return (1);
}

int
main(int argc, char **argv)
{
	int error = 0;
	int i = 0;
	char *cmdname;
	char **newargv;

	(void) setlocale(LC_ALL, "");
	(void) setlocale(LC_NUMERIC, "C");
	(void) textdomain(TEXT_DOMAIN);

	opterr = 0;

	/*
	 * Make sure the user has specified some command.
	 */
	if (argc < 2) {
		(void) fprintf(stderr, gettext("missing command\n"));
		usage(B_FALSE);
	}

	cmdname = argv[1];

	if ((g_zfs = libzfs_init()) == NULL) {
		(void) fprintf(stderr, "%s\n", libzfs_error_init(errno));
		return (1);
	}

	libzfs_print_on_error(g_zfs, B_TRUE);

	char *random_path = "/dev/urandom";
	uzfs_fd_rand = open(random_path, O_RDONLY);
	ASSERT3S(uzfs_fd_rand, >=, 0);


	/*
	 * Many commands modify input strings for string parsing reasons.
	 * We create a copy to protect the original argv.
	 */
	newargv = malloc((argc + 1) * sizeof (newargv[0]));
	for (i = 0; i < argc; i++)
		newargv[i] = strdup(argv[i]);
	newargv[argc] = NULL;

	/*
	 * Run the appropriate command.
	 */
	libzfs_mnttab_cache(g_zfs, B_TRUE);
	if (find_command_idx(cmdname, &i) == 0) {
		current_command = &command_table[i];
		error = command_table[i].func(argc - 1, newargv + 1);
	} else if (strchr(cmdname, '=') != NULL) {
		verify(find_command_idx("set", &i) == 0);
		current_command = &command_table[i];
		error = command_table[i].func(argc, newargv);
	} else {
		(void) fprintf(stderr, gettext("unrecognized "
		    "command '%s'\n"), cmdname);
		usage(B_FALSE);
		error = 1;
	}

	for (i = 0; i < argc; i++)
		free(newargv[i]);
	free(newargv);

	libzfs_fini(g_zfs);

	/*
	 * The 'ZFS_ABORT' environment variable causes us to dump core on exit
	 * for the purposes of running ::findleaks.
	 */
	if (getenv("ZFS_ABORT") != NULL) {
		(void) printf("dumping core by request\n");
		abort();
	}

	return (error);
}

static void print_stat(const char* name, struct stat* stat)
{
    const char* format =
        "  File: %s(%s)\n"
        "  Inode: %lld\n"
        "  MODE: %x\n"
        "  Links: %lld\n"
        "  UID/GID: %d/%d\n"
        "  SIZE: %d\n"
        "  BLOCKSIZE: %d\n"
        "  BLOCKS: %d\n";

    const char* type = S_ISDIR(stat->st_mode) ? "DIR" : "REG FILE";

    printf(format, name, type, stat->st_ino, stat->st_mode, stat->st_nlink, stat->st_uid, stat->st_gid,
           stat->st_size, stat->st_blksize, stat->st_blocks);
}

static int
uzfs_do_stat(int argc, char **argv)
{
    int error = 0;
    char *path = argv[1];

    char fsname[256] = "";
    char target_path[256] = "";

    char *fs_end = strstr(path, "://");
    memcpy(fsname, path, fs_end - path);
    memcpy(target_path, fs_end + 3, strlen(path) - strlen(fsname) - 3);

    printf("stat %s: %s\n", fsname, target_path);

    uint64_t fsid = 0;
    error = uzfs_init(fsname, &fsid);
    if (error) goto out;

    uint64_t root_ino = 0;

    error = uzfs_getroot(fsid, &root_ino);
    if (error) goto out;

    char *s = target_path;
    if (*s != '/') {
        printf("path %s must be started with /\n", target_path);
        error = 1;
        goto out;
    }

    s++;

    char *e = strchr(s, '/');

    uint64_t dino = root_ino;
    uint64_t ino = 0;
    boolean_t out_flag = B_FALSE;

    while (s) {
        if (e)
            *e = '\0';
        else
            out_flag = B_TRUE;

        error = uzfs_lookup(fsid, dino, s, &ino);
        if (error) goto out;

        if (out_flag)
            break;

        s = e + 1;
        e = strchr(s, '/');
        dino = ino;
    }

    if (ino == 0) {
        printf("Empty path\n");
        error = 1;
        goto out;
    }

    struct stat buf;
    memset(&buf, 0, sizeof(struct stat));

    error = uzfs_getattr(fsid, ino, &buf);
    if (error) {
        printf("Failed to stat %s\n", path);
        goto out;
    }

    print_stat(s, &buf);

out:

    uzfs_fini(fsid);

    return error;
}

static int
uzfs_do_read(int argc, char **argv)
{
    int error = 0;
    char buf[4096] = "";
    char *path = argv[1];
    int offset = atoi(argv[2]);
    int size = atoi(argv[3]);

    char fsname[256] = "";
    char target_path[256] = "";

    char *fs_end = strstr(path, "://");
    memcpy(fsname, path, fs_end - path);
    memcpy(target_path, fs_end + 3, strlen(path) - strlen(fsname) - 3);

    printf("read %s: %s, offset: %d, size: %d\n", fsname, target_path, offset, size);

    uint64_t fsid = 0;
    error = uzfs_init(fsname, &fsid);
    if (error) goto out;

    uint64_t root_ino = 0;

    error = uzfs_getroot(fsid, &root_ino);
    if (error) goto out;

    char *s = target_path;
    if (*s != '/') {
        printf("path %s must be started with /\n", target_path);
        error = 1;
        goto out;
    }

    s++;

    char *e = strchr(s, '/');

    uint64_t dino = root_ino;
    uint64_t ino = 0;
    boolean_t out_flag = B_FALSE;

    while (s) {
        if (e)
            *e = '\0';
        else
            out_flag = B_TRUE;

        error = uzfs_lookup(fsid, dino, s, &ino);
        if (error) goto out;

        if (out_flag)
            break;

        s = e + 1;
        e = strchr(s, '/');
        dino = ino;
    }

    if (ino == 0) {
        printf("Empty path\n");
        error = 1;
        goto out;
    }

    struct iovec iov;
    iov.iov_base = buf;
    iov.iov_len = size;

    zfs_uio_t uio;
    zfs_uio_iovec_init(&uio, &iov, 1, offset, UIO_USERSPACE, iov.iov_len, 0);

    error = uzfs_read(fsid, ino, &uio, 0);
    if (error) {
        printf("Failed to read %s\n", path);
        goto out;
    }

    printf("%s\n", buf);

    uzfs_fini(fsid);

out:


    return error;
}

static int
uzfs_do_write(int argc, char **argv)
{
    int error = 0;
    char buf[4096] = "";
    char *path = argv[1];
    int offset = atoi(argv[2]);
    int size = atoi(argv[3]);

    char fsname[256] = "";
    char target_path[256] = "";

    char *fs_end = strstr(path, "://");
    memcpy(fsname, path, fs_end - path);
    memcpy(target_path, fs_end + 3, strlen(path) - strlen(fsname) - 3);
    memcpy(buf, argv[4], strlen(argv[4]));

    printf("write %s: %s\n", fsname, target_path);

    uint64_t fsid = 0;
    error = uzfs_init(fsname, &fsid);
    if (error) goto out;

    uint64_t root_ino = 0;

    error = uzfs_getroot(fsid, &root_ino);
    if (error) goto out;

    char *s = target_path;
    if (*s != '/') {
        printf("path %s must be started with /\n", target_path);
        error = 1;
        goto out;
    }

    s++;

    char *e = strchr(s, '/');

    uint64_t dino = root_ino;
    uint64_t ino = 0;
    boolean_t out_flag = B_FALSE;

    while (s) {
        if (e)
            *e = '\0';
        else
            out_flag = B_TRUE;

        error = uzfs_lookup(fsid, dino, s, &ino);
        if (error) goto out;

        if (out_flag)
            break;

        s = e + 1;
        e = strchr(s, '/');
        dino = ino;
    }

    if (ino == 0) {
        printf("Empty path\n");
        error = 1;
        goto out;
    }

    struct iovec iov;
    iov.iov_base = buf;
    iov.iov_len = size;

    zfs_uio_t uio;
    zfs_uio_iovec_init(&uio, &iov, 1, offset, UIO_USERSPACE, iov.iov_len, 0);

    error = uzfs_write(fsid, ino, &uio, 0);
    if (error) {
        printf("Failed to write %s\n", path);
        goto out;
    }

    uzfs_fini(fsid);

out:


    return error;
}

static int
uzfs_do_fsync(int argc, char **argv)
{
    int error = 0;
    char *path = argv[1];

    char fsname[256] = "";
    char target_path[256] = "";

    char *fs_end = strstr(path, "://");
    memcpy(fsname, path, fs_end - path);
    memcpy(target_path, fs_end + 3, strlen(path) - strlen(fsname) - 3);

    printf("write %s: %s\n", fsname, target_path);

    uint64_t fsid = 0;
    error = uzfs_init(fsname, &fsid);
    if (error) goto out;

    uint64_t root_ino = 0;

    error = uzfs_getroot(fsid, &root_ino);
    if (error) goto out;

    char *s = target_path;
    if (*s != '/') {
        printf("path %s must be started with /\n", path);
        error = 1;
        goto out;
    }

    s++;

    char *e = strchr(s, '/');

    uint64_t dino = root_ino;
    uint64_t ino = 0;
    boolean_t out_flag = B_FALSE;

    while (s) {
        if (e)
            *e = '\0';
        else
            out_flag = B_TRUE;

        error = uzfs_lookup(fsid, dino, s, &ino);
        if (error) goto out;

        if (out_flag)
            break;

        s = e + 1;
        e = strchr(s, '/');
        dino = ino;
    }

    if (ino == 0) {
        printf("Empty path\n");
        error = 1;
        goto out;
    }

    error = uzfs_fsync(fsid, ino, 0);
    if (error) {
        printf("Failed to sync %s\n", path);
        goto out;
    }

    uzfs_fini(fsid);

out:

    return error;
}


static int
uzfs_do_setxattr(int argc, char **argv)
{
    int error = 0;
    char *path = argv[1];

    char fsname[256] = "";
    char target_path[256] = "";

    char *fs_end = strstr(path, "://");
    memcpy(fsname, path, fs_end - path);
    memcpy(target_path, fs_end + 3, strlen(path) - strlen(fsname) - 3);

    char *name = argv[2];
    char *value = argv[3];

    printf("setxattr %s: %s, name: %s, value: %s\n", fsname, target_path, name, value);

    uint64_t fsid = 0;
    error = uzfs_init(fsname, &fsid);
    if (error) goto out;

    uint64_t root_ino = 0;

    error = uzfs_getroot(fsid, &root_ino);
    if (error) goto out;

    char *s = target_path;
    if (*s != '/') {
        printf("path %s must be started with /\n", target_path);
        error = 1;
        goto out;
    }

    s++;

    char *e = strchr(s, '/');

    uint64_t dino = root_ino;
    uint64_t ino = 0;
    boolean_t out_flag = B_FALSE;


    while (s) {
        if (e)
            *e = '\0';
        else
            out_flag = B_TRUE;

        error = uzfs_lookup(fsid, dino, s, &ino);
        if (error) goto out;

        if (out_flag)
            break;

        s = e + 1;
        e = strchr(s, '/');
        dino = ino;
    }

    if (ino == 0) {
        printf("Empty path\n");
        error = 1;
        goto out;
    }

    error = uzfs_setxattr(fsid, ino, name, value, sizeof(value), 0);
    if (error) {
        printf("Failed to setxattr %s\n", path);
        goto out;
    }

out:

    uzfs_fini(fsid);

    return error;
}


static int
uzfs_do_getxattr(int argc, char **argv)
{
    int error = 0;
    char *path = argv[1];

    char fsname[256] = "";
    char target_path[256] = "";

    char *fs_end = strstr(path, "://");
    memcpy(fsname, path, fs_end - path);
    memcpy(target_path, fs_end + 3, strlen(path) - strlen(fsname) - 3);

    char *name = argv[2];

    printf("getxattr %s: %s, name: %s\n", fsname, target_path, name);

    uint64_t fsid = 0;
    error = uzfs_init(fsname, &fsid);
    if (error) goto out;

    uint64_t root_ino = 0;

    error = uzfs_getroot(fsid, &root_ino);
    if (error) goto out;

    char *s = target_path;
    if (*s != '/') {
        printf("path %s must be started with /\n", target_path);
        error = 1;
        goto out;
    }

    s++;

    char *e = strchr(s, '/');

    uint64_t dino = root_ino;
    uint64_t ino = 0;
    boolean_t out_flag = B_FALSE;

    while (s) {
        if (e)
            *e = '\0';
        else
            out_flag = B_TRUE;

        error = uzfs_lookup(fsid, dino, s, &ino);
        if (error) goto out;

        if (out_flag)
            break;

        s = e + 1;
        e = strchr(s, '/');
        dino = ino;
    }

    if (ino == 0) {
        printf("Empty path\n");
        error = 1;
        goto out;
    }

    char value[256] = "";
    int size = -1;

    size = uzfs_getxattr(fsid, ino, name, &value, sizeof(value));
    if (size <= 0) {
        printf("Failed to getxattr %s\n", path);
        goto out;
    }

    printf("%s xattr:\n\t%s:%s\n", path, name, value);

out:

    uzfs_fini(fsid);

    return error;
}

static int
uzfs_do_mkdir(int argc, char **argv)
{
    int error = 0;
    char *path = argv[1];

    char fsname[256] = "";
    char target_path[256] = "";

    char *fs_end = strstr(path, "://");
    memcpy(fsname, path, fs_end - path);
    memcpy(target_path, fs_end + 3, strlen(path) - strlen(fsname) - 3);

    printf("mkdir %s: %s\n", fsname, target_path);

    uint64_t fsid = 0;
    error = uzfs_init(fsname, &fsid);
    if (error) goto out;

    uint64_t root_ino = 0;

    error = uzfs_getroot(fsid, &root_ino);
    if (error) goto out;

    char *s = target_path;
    if (*s != '/') {
        printf("path %s must be started with /\n", target_path);
        error = 1;
        goto out;
    }

    s++;

    char *e = strchr(s, '/');

    uint64_t dino = root_ino;
    uint64_t ino = 0;

    while (e) {
        *e = '\0';

        error = uzfs_lookup(fsid, dino, s, &ino);
        if (error) goto out;

        s = e + 1;
        e = strchr(s, '/');
        dino = ino;
    }

    error = uzfs_mkdir(fsid, dino, s, 0, &ino);
    if (error) {
        printf("Failed to mkdir %s\n", path);
        goto out;
    }

    printf("succeeded to mkdir %s\n", path);

out:

    uzfs_fini(fsid);

    return error;

}

static int
uzfs_do_create(int argc, char **argv)
{
    int error = 0;
    char *path = argv[1];

    char fsname[256] = "";
    char target_path[256] = "";

    char *fs_end = strstr(path, "://");
    memcpy(fsname, path, fs_end - path);
    memcpy(target_path, fs_end + 3, strlen(path) - strlen(fsname) - 3);

    printf("create %s: %s\n", fsname, target_path);


    uint64_t fsid = 0;
    error = uzfs_init(fsname, &fsid);
    if (error) goto out;

    uint64_t root_ino = 0;

    error = uzfs_getroot(fsid, &root_ino);
    if (error) goto out;

    char *s = target_path;
    if (*s != '/') {
        printf("path %s must be started with /\n", target_path);
        error = 1;
        goto out;
    }

    s++;

    char *e = strchr(s, '/');

    uint64_t dino = root_ino;
    uint64_t ino = 0;

    while (e) {
        *e = '\0';

        error = uzfs_lookup(fsid, dino, s, &ino);
        if (error) goto out;

        s = e + 1;
        e = strchr(s, '/');
        dino = ino;
    }

    error = uzfs_create(fsid, dino, s, 0, &ino);
    if (error) {
        printf("Failed to create file %s\n", path);
        goto out;
    }

out:

    uzfs_fini(fsid);

    return error;
}


static int
uzfs_do_rm(int argc, char **argv)
{
    int error = 0;
    char *path = argv[1];

    char fsname[256] = "";
    char target_path[256] = "";

    char *fs_end = strstr(path, "://");
    memcpy(fsname, path, fs_end - path);
    memcpy(target_path, fs_end + 3, strlen(path) - strlen(fsname) - 3);

    printf("rm %s: %s\n", fsname, target_path);

    uint64_t fsid = 0;
    error = uzfs_init(fsname, &fsid);
    if (error) goto out;

    uint64_t root_ino = 0;

    error = uzfs_getroot(fsid, &root_ino);
    if (error) goto out;

    char *s = target_path;
    if (*s != '/') {
        printf("path %s must be started with /\n", target_path);
        error = 1;
        goto out;
    }

    s++;

    char *e = strchr(s, '/');

    uint64_t dino = root_ino;
    uint64_t ino = 0;
    boolean_t out_flag = B_FALSE;

    while (s) {
        if (e)
            *e = '\0';
        else
            out_flag = B_TRUE;

        error = uzfs_lookup(fsid, dino, s, &ino);
        if (error) goto out;

        if (out_flag)
            break;

        s = e + 1;
        e = strchr(s, '/');
        dino = ino;
    }

    if (ino == 0) {
        printf("Empty path\n");
        error = 1;
        goto out;
    }

    struct stat buf;
    memset(&buf, 0, sizeof(struct stat));

    error = uzfs_getattr(fsid, ino, &buf);
    if (error) {
        printf("Failed to stat %s\n", path);
        goto out;
    }

    if (S_ISDIR(buf.st_mode)) {
        error = uzfs_rmdir(fsid, dino, s);
        if (error) {
            printf("Failed to rm dir %s\n", path);
            goto out;
        }
    } else if (S_ISREG(buf.st_mode)) {
        error = uzfs_remove(fsid, dino, s);
        if (error) {
            printf("Failed to rm file %s\n", path);
            goto out;
        }
    } else {
        printf("Invalid file type\n");
        error = 1;
        goto out;
    }

out:

    uzfs_fini(fsid);

    return error;
}

static int
uzfs_do_ls(int argc, char **argv)
{
    int error = 0;
    char *path = argv[1];

    char fsname[256] = "";
    char target_path[256] = "";

    char *fs_end = strstr(path, "://");
    memcpy(fsname, path, fs_end - path);
    memcpy(target_path, fs_end + 3, strlen(path) - strlen(fsname) - 3);

    printf("ls %s: %s\n", fsname, target_path);

    uint64_t fsid = 0;
    error = uzfs_init(fsname, &fsid);
    if (error) goto out;

    uint64_t root_ino = 0;

    error = uzfs_getroot(fsid, &root_ino);
    if (error) goto out;

    char *s = target_path;
    if (*s != '/') {
        printf("path %s must be started with /\n", target_path);
        error = 1;
        goto out;
    }

    s++;

    char *e = strchr(s, '/');

    uint64_t dino = root_ino;
    uint64_t ino = 0;
    boolean_t out_flag = B_FALSE;

    while (s) {
        if (e)
            *e = '\0';
        else
            out_flag = B_TRUE;

        error = uzfs_lookup(fsid, dino, s, &ino);
        if (error) goto out;

        if (out_flag)
            break;

        s = e + 1;
        e = strchr(s, '/');
        dino = ino;
    }

    if (ino == 0) {
        printf("Empty path\n");
        error = 1;
        goto out;
    }

    struct stat buf;
    memset(&buf, 0, sizeof(struct stat));

    error = uzfs_getattr(fsid, ino, &buf);
    if (error) {
        printf("Failed to stat %s\n", path);
        goto out;
    }

    if (S_ISDIR(buf.st_mode)) {
        error = uzfs_readdir(fsid, ino, NULL, 0);
        if (error) {
            printf("Failed to readdir %s\n", path);
            goto out;
        }
    } else if (S_ISREG(buf.st_mode)) {
        printf("%s\t%ld\n", s, buf.st_ino);
    } else {
        printf("Invalid file type\n");
        error = 1;
        goto out;
    }

out:

    uzfs_fini(fsid);

    return error;
}

static int
uzfs_do_truncate(int argc, char **argv)
{
    int error = 0;
    char *path = argv[1];
    int size = atoi(argv[2]);

    char fsname[256] = "";
    char target_path[256] = "";

    char *fs_end = strstr(path, "://");
    memcpy(fsname, path, fs_end - path);
    memcpy(target_path, fs_end + 3, strlen(path) - strlen(fsname) - 3);

    printf("truncate %s: %s\n", fsname, target_path);

    uint64_t fsid = 0;
    error = uzfs_init(fsname, &fsid);
    if (error) goto out;

    uint64_t root_ino = 0;

    error = uzfs_getroot(fsid, &root_ino);
    if (error) goto out;

    char *s = target_path;
    if (*s != '/') {
        printf("path %s must be started with /\n", target_path);
        error = 1;
        goto out;
    }

    s++;

    char *e = strchr(s, '/');

    uint64_t dino = root_ino;
    uint64_t ino = 0;
    boolean_t out_flag = B_FALSE;

    while (s) {
        if (e)
            *e = '\0';
        else
            out_flag = B_TRUE;

        error = uzfs_lookup(fsid, dino, s, &ino);
        if (error) goto out;

        if (out_flag)
            break;

        s = e + 1;
        e = strchr(s, '/');
        dino = ino;
    }

    if (ino == 0) {
        printf("Empty path\n");
        error = 1;
        goto out;
    }

    struct stat buf;
    memset(&buf, 0, sizeof(struct stat));

    error = uzfs_getattr(fsid, ino, &buf);
    if (error) {
        printf("Failed to stat %s\n", path);
        goto out;
    }

    if (buf.st_size != size) {
        struct iattr ia;
        memset(&ia, 0, sizeof(ia));
        ia.ia_size = size;
        ia.ia_valid = ATTR_SIZE;
        uzfs_setattr(fsid, ino, &ia);
    }

out:

    uzfs_fini(fsid);

    return error;
}

static int
uzfs_do_fallocate(int argc, char **argv)
{
    int error = 0;
    char *path = argv[1];
    int offset = atoi(argv[2]);
    int size = atoi(argv[3]);
    int mode = atoi(argv[4]);

    char fsname[256] = "";
    char target_path[256] = "";

    char *fs_end = strstr(path, "://");
    memcpy(fsname, path, fs_end - path);
    memcpy(target_path, fs_end + 3, strlen(path) - strlen(fsname) - 3);

    printf("fallocate %s: %s\n", fsname, target_path);

    uint64_t fsid = 0;
    error = uzfs_init(fsname, &fsid);
    if (error) goto out;

    uint64_t root_ino = 0;

    error = uzfs_getroot(fsid, &root_ino);
    if (error) goto out;

    char *s = target_path;
    if (*s != '/') {
        printf("path %s must be started with /\n", target_path);
        error = 1;
        goto out;
    }

    s++;

    char *e = strchr(s, '/');

    uint64_t dino = root_ino;
    uint64_t ino = 0;
    boolean_t out_flag = B_FALSE;

    while (s) {
        if (e)
            *e = '\0';
        else
            out_flag = B_TRUE;

        error = uzfs_lookup(fsid, dino, s, &ino);
        if (error) goto out;

        if (out_flag)
            break;

        s = e + 1;
        e = strchr(s, '/');
        dino = ino;
    }

    if (ino == 0) {
        printf("Empty path\n");
        error = 1;
        goto out;
    }

    error = uzfs_fallocate(fsid, ino, mode, offset, size);
    if (error) {
        printf("Failed to stat %s\n", path);
        goto out;
    }

out:

    uzfs_fini(fsid);

    return error;
}

static int
uzfs_do_mv(int argc, char **argv)
{
    int error = 0;
    char *spath = argv[1];
    char *dst_path = argv[2];

    char fsname[256] = "";
    char src_path[256] = "";

    char *fs_end = strstr(spath, "://");
    memcpy(fsname, spath, fs_end - spath);
    memcpy(src_path, fs_end + 3, strlen(spath) - strlen(fsname) - 3);

    printf("mv %s: %s %s\n", fsname, src_path, dst_path);

    uint64_t fsid = 0;
    error = uzfs_init(fsname, &fsid);
    if (error) goto out;

    uint64_t root_ino = 0;

    error = uzfs_getroot(fsid, &root_ino);
    if (error) goto out;

    char *s = src_path;
    if (*s != '/') {
        printf("path %s must be started with /\n", src_path);
        error = 1;
        goto out;
    }

    s++;

    char *e = strchr(s, '/');

    uint64_t sdino = root_ino;
    uint64_t sino = 0;

    while (e) {
        *e = '\0';

        error = uzfs_lookup(fsid, sdino, s, &sino);
        if (error) goto out;

        s = e + 1;
        e = strchr(s, '/');
        sdino = sino;
    }

    char *d = dst_path;
    if (*d != '/') {
        printf("path %s must be started with /\n", dst_path);
        error = 1;
        goto out;
    }

    d++;

    e = strchr(d, '/');

    uint64_t dst_dino = root_ino;
    uint64_t dst_ino = 0;

    while (e) {
        *e = '\0';

        error = uzfs_lookup(fsid, dst_dino, d, &dst_ino);
        if (error) goto out;

        d = e + 1;
        e = strchr(d, '/');
        dst_dino = dst_ino;
    }


    error = uzfs_rename(fsid, sdino, s, dst_dino, d);
    if (error) {
        printf("Failed to mv %s %s\n", spath, dst_path);
        goto out;
    }

out:

    uzfs_fini(fsid);

    return error;

}


struct perf_args {
    uint64_t fsid;
    uint64_t dino;
    int op;
    int num;
    int tid;
};

static void* do_perf(void* perf_args)
{
    struct perf_args* args = perf_args;
    uint64_t fsid = args->fsid;
    uint64_t root_ino = args->dino;
    int op = args->op;
    int num = args->num;
    int tid = args->tid;
    int error = 0;
    int i = 0;
    uint64_t ino;
    uint64_t dino;
    char name[20] = "";

    printf("tid: %d\n", tid);
    sprintf(name, "t%d", tid);

    if (op == 1 || op == 3) {
        error = uzfs_mkdir(fsid, root_ino, name, 0, &ino);
        if (error) {
            printf("Failed to mkdir parent %s\n", name);
            goto out;
        }
    } else {
        error = uzfs_lookup(fsid, root_ino, name, &ino);
        if (error) {
            printf("Failed to lookup parent dir %s\n", name);
            goto out;
        }
    }

    dino = ino;

    int print_idx = num / 100;
    for (i = 0; i < num; i++) {
        sprintf(name, "%d", i);
        if (op == 0) {
            error = uzfs_rmdir(fsid, dino, name);
            if (error) {
                printf("Failed to mkdir %s\n", name);
                goto out;
            }
        } else if (op == 1) {
            error = uzfs_mkdir(fsid, dino, name, 0, &ino);
            if (error) {
                printf("Failed to rmdir %s\n", name);
                goto out;
            }
        } else if (op == 2) {
            error = uzfs_remove(fsid, dino, name);
            if (error) {
                printf("Failed to remove file %s\n", name);
                goto out;
            }
        } else if (op == 3) {
            error = uzfs_create(fsid, dino, name, 0, &ino);
            if (error) {
                printf("Failed to create file %s\n", name);
                goto out;
            }
        } else if (op == 4) {
            error = uzfs_lookup(fsid, dino, name, &ino);
            if (error) goto out;

            struct stat buf;
            memset(&buf, 0, sizeof(struct stat));

            error = uzfs_getattr(fsid, ino, &buf);
            if (error) {
                printf("Failed to stat %s\n", name);
                goto out;
            }
            //print_stat(name, &buf);
        }
        if (print_idx != 0 && i % print_idx == 0) {
            printf("tid %d: %d%%\n", tid, i / print_idx);
        }
    }

    if (op == 0 || op == 2) {
        sprintf(name, "t%d", tid);
        error = uzfs_rmdir(fsid, root_ino, name);
        if (error) {
            printf("Failed to rm parent dir %s\n", name);
            goto out;
        }
    }

    printf("tid: %d done\n", tid);


out:
    return NULL;
}

static int
uzfs_perf(int argc, char **argv)
{
    int error = 0;
    char *path = argv[1];
    int op = atoi(argv[2]); // 0: rmdir, 1: mkdir, 2: remove file, 3: create file, 4: stat
    int depth = atoi(argv[3]);
    int branch = atoi(argv[4]);
    int num = atoi(argv[5]);
    int n_threads = atoi(argv[6]);

    char fsname[256] = "";
    char target_path[256] = "";
    char *opstr = NULL;

    char *fs_end = strstr(path, "://");
    memcpy(fsname, path, fs_end - path);
    memcpy(target_path, fs_end + 3, strlen(path) - strlen(fsname) - 3);

    switch (op) {
        case 0:
            opstr = "rmdir";
            break;
        case 1:
            opstr = "mkdir";
            break;
        case 2:
            opstr = "rm file";
            break;
        case 3:
            opstr = "create file";
            break;
        case 4:
            opstr = "stat";
            break;
        default:
            printf("invalid op: %d\n", op);
            return -1;
    }

    printf("%s %s: %s\n", opstr, fsname, target_path);

    uint64_t fsid = 0;
    error = uzfs_init(fsname, &fsid);
    if (error) goto out;

    uint64_t root_ino = 0;

    error = uzfs_getroot(fsid, &root_ino);
    if (error) goto out;

    char *s = target_path;
    if (*s != '/') {
        printf("path %s must be started with /\n", target_path);
        error = 1;
        goto out;
    }

    s++;

    char *e = strchr(s, '/');

    uint64_t dino = root_ino;
    uint64_t ino = 0;

    while (e) {
        *e = '\0';

        error = uzfs_lookup(fsid, dino, s, &ino);
        if (error) goto out;

        s = e + 1;
        e = strchr(s, '/');
        dino = ino;
    }

    struct timeval t1,t2;
    double timeuse;
    gettimeofday(&t1,NULL);

    int i;
    clock_t start, end;
    start = clock();
    pthread_t ntids[100];
    struct perf_args args[100];
    for (i = 0; i < n_threads; i++) {
        args[i].fsid = fsid;
        args[i].dino = dino;
        args[i].op = op;
        args[i].num = num;
        args[i].tid = i;
        error = pthread_create(&ntids[i], NULL, do_perf, (void*)&args[i]);
        if  (error != 0) {
            printf("Failed to create thread: %s\n" ,  strerror (error));
            goto out;
        }

    }
    for (i = 0; i < n_threads; i++) {
        pthread_join(ntids[i],NULL);
    }

    end = clock();
    gettimeofday(&t2,NULL);
    timeuse = (t2.tv_sec - t1.tv_sec) + (double)(t2.tv_usec - t1.tv_usec)/1000000.0;

    int totalnum = branch * depth * num * n_threads;
    double clockuse = ((double)(end - start))/CLOCKS_PER_SEC;
    double rate = totalnum / timeuse;
    printf("num: %d\ntime=%fs\nclock=%fs\nrate=%f\n", totalnum, timeuse, clockuse, rate);

out:

    uzfs_fini(fsid);

    return error;

}

#include <sys/spa.h>
#include <sys/dmu.h>
#include <sys/txg.h>
#include <sys/dbuf.h>
#include <sys/zap.h>
#include <sys/dmu_objset.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/zio.h>
#include <sys/zil.h>
#include <sys/zil_impl.h>
#include <sys/vdev_draid.h>
#include <sys/vdev_impl.h>
#include <sys/vdev_file.h>
#include <sys/vdev_initialize.h>
#include <sys/vdev_raidz.h>
#include <sys/vdev_trim.h>
#include <sys/spa_impl.h>
#include <sys/metaslab_impl.h>
#include <sys/dsl_prop.h>
#include <sys/dsl_dataset.h>
#include <sys/dsl_destroy.h>
#include <sys/dsl_scan.h>
#include <sys/zio_checksum.h>
#include <sys/zfs_refcount.h>
#include <sys/zfeature.h>
#include <sys/dsl_userhold.h>
#include <sys/abd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <umem.h>
#include <ctype.h>
#include <math.h>
#include <sys/fs/zfs.h>
#include <zfs_fletcher.h>
#include <libnvpair.h>
#include <libzutil.h>
#include <sys/crypto/icp.h>
#if (__GLIBC__ && !__UCLIBC__)
#include <execinfo.h> /* for backtrace() */
#endif

static spa_t *uzfs_spa = NULL;

static char *
uzfs_ot_name(dmu_object_type_t type)
{
	if (type < DMU_OT_NUMTYPES)
		return (dmu_ot[type].ot_name);
	else if ((type & DMU_OT_NEWTYPE) &&
	    ((type & DMU_OT_BYTESWAP_MASK) < DMU_BSWAP_NUMFUNCS))
		return (dmu_ot_byteswap[type & DMU_OT_BYTESWAP_MASK].ob_name);
	else
		return ("UNKNOWN");
}

static void uzfs_dump_doi(uint64_t object, dmu_object_info_t *doi)
{

    printf("object: %ld\n", object);
    printf("\tdata_block_size: %d\n", doi->doi_data_block_size);
    printf("\tmetadata_block_size: %d\n", doi->doi_metadata_block_size);
    printf("\ttype: %s\n", uzfs_ot_name(doi->doi_type));
    printf("\tbonus_type: %s\n", uzfs_ot_name(doi->doi_bonus_type));
    printf("\tbonus_size: %ld\n", doi->doi_bonus_size);
    printf("\tindirection: %d\n", doi->doi_indirection);
    printf("\tchecksum: %d\n", doi->doi_checksum);
    printf("\tcompress: %d\n", doi->doi_compress);
    printf("\tnblkptr: %d\n", doi->doi_nblkptr);
    printf("\tdnodesize: %ld\n", doi->doi_dnodesize);
    printf("\tphysical_blocks_512: %ld\n", doi->doi_physical_blocks_512);
    printf("\tmax_offset: %ld\n", doi->doi_max_offset);
    printf("\tfill_count: %ld\n", doi->doi_fill_count);
}

static uint64_t
uzfs_random(uint64_t range)
{
	uint64_t r;

	ASSERT3S(uzfs_fd_rand, >=, 0);

	if (range == 0)
		return (0);

	if (read(uzfs_fd_rand, &r, sizeof (r)) != sizeof (r))
        ASSERT(0);

	return (r % range);
}


static int
uzfs_random_blocksize(void)
{
	ASSERT3U(uzfs_spa->spa_max_ashift, !=, 0);

	/*
	 * Choose a block size >= the ashift.
	 * If the SPA supports new MAXBLOCKSIZE, test up to 1MB blocks.
	 */
	int maxbs = SPA_OLD_MAXBLOCKSHIFT;
	if (spa_maxblocksize(uzfs_spa) == SPA_MAXBLOCKSIZE)
		maxbs = 20;
	uint64_t block_shift =
	    uzfs_random(maxbs - uzfs_spa->spa_max_ashift + 1);
	return (1 << (SPA_MINBLOCKSHIFT + block_shift));
}

static int
uzfs_random_dnodesize(void)
{
	int slots;
	int max_slots = spa_maxdnodesize(uzfs_spa) >> DNODE_SHIFT;

	if (max_slots == DNODE_MIN_SLOTS)
		return (DNODE_MIN_SIZE);

	/*
	 * Weight the random distribution more heavily toward smaller
	 * dnode sizes since that is more likely to reflect real-world
	 * usage.
	 */
	ASSERT3U(max_slots, >, 4);
	switch (uzfs_random(10)) {
	case 0:
		slots = 5 + uzfs_random(max_slots - 4);
		break;
	case 1 ... 4:
		slots = 2 + uzfs_random(3);
		break;
	default:
		slots = 1;
		break;
	}

	return (slots << DNODE_SHIFT);
}

static int
uzfs_random_ibshift(void)
{
	return (DN_MIN_INDBLKSHIFT +
	    uzfs_random(DN_MAX_INDBLKSHIFT - DN_MIN_INDBLKSHIFT + 1));
}

static int uzfs_do_ostat(objset_t *os, uint64_t obj, boolean_t slience)
{
    dmu_object_info_t doi;
    dmu_buf_t *db;

    int error = 0;

    error = dmu_bonus_hold(os, obj, FTAG, &db);
    if (error)
        return error;

    dmu_object_info_from_db(db, &doi);

    if (!slience)
        uzfs_dump_doi(obj, &doi);

    dmu_buf_rele(db, FTAG);
    return 0;
}

static int uzfs_do_olist(objset_t *os)
{
    uint64_t obj;
    int err;
    int i = 0;

	for (obj = 0; err == 0; err = dmu_object_next(os, &obj, B_FALSE, 0)) {
        if (uzfs_do_ostat(os, obj, B_FALSE)) {
            printf("skip obj w/o bonus buf: %ld\n", obj);
            continue;
        }
        i++;
    }

    return i;
}

static int
uzfs_olist(int argc, char **argv)
{
    int error = 0;
    char *dsname = argv[1];

    printf("olist %s\n", dsname);

    objset_t *os = NULL;

	error = dmu_objset_own(dsname, DMU_OST_ZFS, B_FALSE, B_TRUE, FTAG, &os);
    if (error != 0) {
        printf("Failed to own objset: %s\n", dsname);
        goto out;
    }

    int num = uzfs_do_olist(os);

    dmu_objset_disown(os, B_TRUE, FTAG);

    printf("olist done, total num: %d\n", num);

out:

    return error;
}

static int
uzfs_ostat(int argc, char **argv)
{
    int error = 0;
    char *dsname = argv[1];
    uint64_t obj = atoi(argv[2]);

    printf("ostat %s:%ld\n", dsname, obj);

    objset_t *os = NULL;

	error = dmu_objset_own(dsname, DMU_OST_ZFS, B_FALSE, B_TRUE, FTAG, &os);
    if (error != 0) {
        printf("Failed to own objset: %s\n", dsname);
        goto out;
    }

    if (uzfs_do_ostat(os, obj, B_FALSE)) {
        printf("skip obj w/o bonus buf: %ld\n", obj);
    }

    dmu_objset_disown(os, B_TRUE, FTAG);

out:

    return error;
}


static int
uzfs_do_ocreate(objset_t *os, uint64_t *obj)
{
    int error = 0;
    dmu_tx_t *tx;
    dmu_buf_t *db;

    tx = dmu_tx_create(os);

    dmu_tx_hold_bonus(tx, DMU_NEW_OBJECT);

	error = dmu_tx_assign(tx, TXG_WAIT);
    if (error) {
        dmu_tx_abort(tx);
        goto out;
    }

    int dnodesize = uzfs_random_dnodesize();
    int bonuslen = DN_BONUS_SIZE(dnodesize);
    int blocksize = uzfs_random_blocksize();
    int ibshift = uzfs_random_ibshift();

    *obj = dmu_object_alloc_dnsize(os, DMU_OT_UINT64_OTHER, 0, DMU_OT_UINT64_OTHER,
                                   bonuslen, dnodesize, tx);

    VERIFY0(dmu_object_set_blocksize(os, *obj, blocksize, ibshift, tx));
    VERIFY0(dmu_bonus_hold(os, *obj, FTAG, &db));
	dmu_buf_will_dirty(db, tx);
	dmu_buf_rele(db, FTAG);
    dmu_tx_commit(tx);

out:
    return error;
}

static int
uzfs_ocreate(int argc, char **argv)
{
    int error = 0;
    char *dsname = argv[1];

    printf("ocreate %s\n", dsname);

    objset_t *os = NULL;

    error = dmu_objset_own(dsname, DMU_OST_ZFS, B_FALSE, B_TRUE, FTAG, &os);
    if (error != 0) {
        printf("Failed to own objset: %s\n", dsname);
        goto out;
    }

    uzfs_spa = os->os_spa;

    uint64_t obj = 0;
    error = uzfs_do_ocreate(os, &obj);
    if (error)
        goto out;

    uzfs_do_ostat(os, obj, B_FALSE);

out:
 if (os)
        dmu_objset_disown(os, B_TRUE, FTAG);

    return error;
}

static int uzfs_do_orm(objset_t *os, uint64_t obj)
{
    int error = 0;
    dmu_tx_t *tx;

    tx = dmu_tx_create(os);

    dmu_tx_hold_free(tx, obj, 0, DMU_OBJECT_END);

	error = dmu_tx_assign(tx, TXG_WAIT);
    if (error) {
        dmu_tx_abort(tx);
        goto out;
    }

	VERIFY0(dmu_object_free(os, obj, tx));

    dmu_tx_commit(tx);

out:
    return error;

}

static int
uzfs_orm(int argc, char **argv)
{
    int error = 0;
    char *dsname = argv[1];
    uint64_t obj = atoi(argv[2]);

    printf("orm %s: %ld\n", dsname, obj);

    objset_t *os = NULL;

	error = dmu_objset_own(dsname, DMU_OST_ZFS, B_FALSE, B_TRUE, FTAG, &os);
    if (error != 0) {
        printf("Failed to own objset: %s\n", dsname);
        goto out;
    }

    error = uzfs_do_ostat(os, obj, B_FALSE);
    if (error)
       goto out;

    error = uzfs_do_orm(os, obj);

out:
    if (os)
        dmu_objset_disown(os, B_TRUE, FTAG);

    return error;
}

static int
uzfs_do_owrite(objset_t *os, uint64_t obj, int offset, int size, char *buf)
{
    int error = 0;
    dmu_tx_t *tx;

    tx = dmu_tx_create(os);

    dmu_tx_hold_write(tx, obj, offset, size);

	error = dmu_tx_assign(tx, TXG_WAIT);
    if (error) {
        dmu_tx_abort(tx);
        goto out;
    }

    dmu_write(os, obj, offset, size, buf, tx);

    dmu_tx_commit(tx);

out:
    return error;
}

static int
uzfs_owrite(int argc, char **argv)
{
    int error = 0;
    char *dsname = argv[1];
    uint64_t obj = atoi(argv[2]);
    int offset = atoi(argv[3]);
    int size = atoi(argv[4]);
    char buf[1024] = "";
    memcpy(buf, argv[5], strlen(argv[5]));

    printf("owrite %s: %ld, off: %d, size: %d\n", dsname, obj, offset, size);

    objset_t *os = NULL;

	error = dmu_objset_own(dsname, DMU_OST_ZFS, B_FALSE, B_TRUE, FTAG, &os);
    if (error != 0) {
        printf("Failed to own objset: %s\n", dsname);
        goto out;
    }

    error = uzfs_do_ostat(os, obj, B_FALSE);
    if (error)
        goto out;

    error = uzfs_do_owrite(os, obj, offset, size, buf);

out:
    if (os)
        dmu_objset_disown(os, B_TRUE, FTAG);

    return error;
}


static int
uzfs_oread(int argc, char **argv)
{
    int error = 0;
    char *dsname = argv[1];
    uint64_t obj = atoi(argv[2]);
    int offset = atoi(argv[3]);
    int size = atoi(argv[4]);
    char buf[1024] = "";

    printf("oread %s: %ld, off: %d, size: %d\n", dsname, obj, offset, size);

    objset_t *os = NULL;

	error = dmu_objset_own(dsname, DMU_OST_ZFS, B_FALSE, B_TRUE, FTAG, &os);
    if (error != 0) {
        printf("Failed to own objset: %s\n", dsname);
        goto out;
    }

    uzfs_spa = os->os_spa;

    error = uzfs_do_ostat(os, obj, B_FALSE);
    if (error)
        goto out;

    dmu_read(os, obj, offset, size, buf, DMU_READ_NO_PREFETCH);

    printf("%s\n", buf);

out:
    if (os)
        dmu_objset_disown(os, B_TRUE, FTAG);

    return error;
}

static int
uzfs_do_oclaim(objset_t *os, uint64_t obj)
{
    int error = 0;
    dmu_tx_t *tx;
    dmu_buf_t *db;

    int dnodesize = 512;
    int bonuslen = DN_BONUS_SIZE(dnodesize);
    int type = DMU_OT_UINT64_OTHER;
    int bonus_type = DMU_OT_UINT64_OTHER;
    int blocksize = 512;
    int ibs = 17; // 128k

    tx = dmu_tx_create(os);

    dmu_tx_hold_bonus(tx, DMU_NEW_OBJECT);

	error = dmu_tx_assign(tx, TXG_WAIT);
    if (error) {
        dmu_tx_abort(tx);
        goto out;
    }

    error = dmu_object_claim_dnsize(os, obj, type, 0, bonus_type, bonuslen, dnodesize, tx);
    if (error)
        goto out;

    VERIFY0(dmu_object_set_blocksize(os, obj, blocksize, ibs, tx));
    VERIFY0(dmu_bonus_hold(os, obj, FTAG, &db));
	dmu_buf_will_dirty(db, tx);
	dmu_buf_rele(db, FTAG);
    dmu_tx_commit(tx);

out:
    return error;
}

static int
uzfs_oclaim(int argc, char **argv)
{
    int error = 0;
    char *dsname = argv[1];
    uint64_t obj = atoi(argv[2]);

    printf("oclaim %s: %ld\n", dsname, obj);

    objset_t *os = NULL;

	error = dmu_objset_own(dsname, DMU_OST_ZFS, B_FALSE, B_TRUE, FTAG, &os);
    if (error != 0) {
        printf("Failed to own objset: %s\n", dsname);
        goto out;
    }

    error = uzfs_do_oclaim(os, obj);
    if (error)
        goto out;

    error = uzfs_do_ostat(os, obj, B_FALSE);
    if (error)
        goto out;

out:
 if (os)
        dmu_objset_disown(os, B_TRUE, FTAG);

    return error;
}


static int
uzfs_do_ocp(objset_t *os, uint64_t obj, dmu_object_info_t *doi)
{
    int error = 0;
    dmu_tx_t *tx;
    dmu_buf_t *db;

    int bonuslen = doi->doi_bonus_size;
    int dnodesize = doi->doi_dnodesize;
    int type = doi->doi_type;
    int bonus_type = doi->doi_bonus_type;
    int blocksize = doi->doi_data_block_size;
    int ibs = doi->doi_metadata_block_size ? ilog2(doi->doi_metadata_block_size) : 0;

    tx = dmu_tx_create(os);

    dmu_tx_hold_bonus(tx, DMU_NEW_OBJECT);

	error = dmu_tx_assign(tx, TXG_WAIT);
    if (error) {
        dmu_tx_abort(tx);
        goto out;
    }

    error = dmu_object_claim_dnsize(os, obj, type, 0, bonus_type, bonuslen, dnodesize, tx);
    if (error)
        goto out;

    VERIFY0(dmu_object_set_blocksize(os, obj, blocksize, ibs, tx));
    VERIFY0(dmu_bonus_hold(os, obj, FTAG, &db));
	dmu_buf_will_dirty(db, tx);
	dmu_buf_rele(db, FTAG);
    dmu_tx_commit(tx);

out:
    return error;
}

static int
uzfs_ocp(int argc, char **argv)
{
    int error = 0;
    char *src = argv[1];
    char *dst = argv[2];
    uint64_t obj = atoi(argv[3]);

    printf("ocp %s: %ld to %s\n", src, obj, dst);

    objset_t *srcos = NULL;
    objset_t *dstos = NULL;

	error = dmu_objset_own(src, DMU_OST_ZFS, B_FALSE, B_TRUE, FTAG, &srcos);
    if (error != 0) {
        printf("Failed to own objset: %s\n", src);
        goto out;
    }

	error = dmu_objset_own(dst, DMU_OST_ZFS, B_FALSE, B_TRUE, FTAG, &dstos);
    if (error != 0) {
        printf("Failed to own objset: %s\n", dst);
        goto out;
    }

    error = uzfs_do_ostat(srcos, obj, B_FALSE);
    if (error)
        goto out;

    dmu_object_info_t doi;
    dmu_object_info(srcos, obj, &doi);

    error = uzfs_do_ocp(dstos, obj, &doi);
    if (error)
        goto out;

    error = uzfs_do_ostat(dstos, obj, B_FALSE);
    if (error)
        goto out;

out:
 if (srcos)
        dmu_objset_disown(srcos, B_TRUE, FTAG);
 if (dstos)
        dmu_objset_disown(dstos, B_TRUE, FTAG);

    return error;
}

struct operf_args {
    objset_t *os;
    int op;
    int num;
    int tid;
};

static void* do_operf(void* operf_args)
{
    struct operf_args* args = operf_args;
    objset_t *os = args->os;
    int op = args->op;
    int num = args->num;
    uint64_t tid = args->tid + 1;
    int error = 0;
    int i = 0;
    uint64_t obj = 0;

    printf("tid: %ld\n", tid);

    int print_idx = num / 100;
    for (i = 0; i < num; i++) {
        obj = tid << 32 | i;
        if (op == 0) {
            error = uzfs_do_orm(os, obj);
            if (error) {
                printf("Failed to rm obj %ld\n", obj);
                goto out;
            }
        } else if (op == 1) {
            error = uzfs_do_oclaim(os, obj);
            if (error) {
                printf("Failed to claim obj %ld\n", obj);
                goto out;
            }
        } else if (op == 2) {
            error = uzfs_do_ostat(os, obj, B_TRUE);
            if (error) {
                printf("Failed to stat obj %ld\n", obj);
                goto out;
            }
        }
        if (print_idx != 0 && i % print_idx == 0) {
            printf("tid %ld: %d%%\n", tid, i / print_idx);
        }
    }
    printf("tid: %ld done\n", tid);

out:
    return NULL;
}


static int
uzfs_operf(int argc, char **argv)
{
    int error = 0;
    char *dsname = argv[1];
    int op = atoi(argv[2]); // 0: rm, 1: create, 2: stat
    int num = atoi(argv[3]);
    int n_threads = atoi(argv[4]);

    char *opstr = NULL;

    switch (op) {
        case 0:
            opstr = "remove";
            break;
        case 1:
            opstr = "create";
            break;
        case 2:
            opstr = "stat";
            break;
        default:
            printf("invalid op: %d\n", op);
            return -1;
    }

    printf("%s %s\n", opstr, dsname);

    objset_t *os;
	error = dmu_objset_own(dsname, DMU_OST_ZFS, B_FALSE, B_TRUE, FTAG, &os);
    if (error != 0) {
        printf("Failed to own objset: %s\n", dsname);
        goto out;
    }

    struct timeval t1,t2;
    double timeuse;
    gettimeofday(&t1,NULL);

    int i;
    clock_t start, end;
    start = clock();
    pthread_t ntids[100];
    struct operf_args args[100];
    for (i = 0; i < n_threads; i++) {
        args[i].os = os;
        args[i].op = op;
        args[i].num = num;
        args[i].tid = i;
        error = pthread_create(&ntids[i], NULL, do_operf, (void*)&args[i]);
        if  (error != 0) {
            printf("Failed to create thread: %s\n" ,  strerror (error));
            goto out;
        }

    }
    for (i = 0; i < n_threads; i++) {
        pthread_join(ntids[i],NULL);
    }

    end = clock();
    gettimeofday(&t2,NULL);
    timeuse = (t2.tv_sec - t1.tv_sec) + (double)(t2.tv_usec - t1.tv_usec)/1000000.0;

    int totalnum = num * n_threads;
    double clockuse = ((double)(end - start))/CLOCKS_PER_SEC;
    double rate = totalnum / timeuse;
    printf("num: %d\ntime=%fs\nclock=%fs\nrate=%f\n", totalnum, timeuse, clockuse, rate);

out:

    if (os)
        dmu_objset_disown(os, B_TRUE, FTAG);

    return error;
}
