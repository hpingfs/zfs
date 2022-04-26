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
#include <sys/file.h>
#include <sys/stat.h>
#include <libzfs.h>
#include <locale.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/zfs_context.h>
#include <libuzfs.h>

libzfs_handle_t *g_zfs;

static int uzfs_do_stat(int argc, char **argv);
static int uzfs_do_read(int argc, char **argv);

typedef enum {
	HELP_STAT,
	HELP_READ,
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
};

#define	NCOMMAND	(sizeof (command_table) / sizeof (command_table[0]))

uzfs_command_t *current_command;

static const char *
get_usage(uzfs_help_t idx)
{
	switch (idx) {
	case HELP_STAT:
		return (gettext("\tstat <path>\n"));
	case HELP_READ:
		return (gettext("\tread ...\n"));
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


static int
uzfs_do_stat(int argc, char **argv)
{
    struct stat statbuf;
    memset(&statbuf, 0, sizeof(struct stat));
	return libuzfs_stat(argv[1], &statbuf);
}

static int
uzfs_do_read(int argc, char **argv)
{
    int error = 0;
    int size = 4096;
    char buf[4096] = "";
    char *path = argv[1];
	zfs_handle_t *zhp;
	int types = ZFS_TYPE_FILESYSTEM;

    char fsname[256] = "";
    char target_path[256] = "";

    char *fs_end = strstr(path, "://");
    memcpy(fsname, path, fs_end - path);
    memcpy(target_path, fs_end + 3, strlen(path) - strlen(fsname) - 3);

    printf("read %s: %s\n", fsname, target_path);

	if ((zhp = libzfs_open(g_zfs, fsname, types)) == NULL)
		return (1);

    error = uzfs_init(fsname);
    if (error) goto out;

    uint64_t root_ino = 0;

    error = uzfs_getroot(fsname, &root_ino);
    if (error) goto out;

    char *s = target_path;
    char *e = strchr(s, '/');
    uint64_t dino = root_ino;
    uint64_t ino = 0;
    boolean_t out_flag = B_FALSE;

    while (s) {
        if (e)
            *e = '\0';
        else
            out_flag = B_TRUE;

        error = uzfs_lookup(dino, s, &ino);
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
    zfs_uio_iovec_init(&uio, &iov, 1, 0, UIO_USERSPACE, iov.iov_len, 0);

    error = uzfs_read(ino, &uio, 0);
    if (error) {
        printf("Failed to read %s\n", target_path);
        goto out;
    }

    printf("%s\n", buf);

    uzfs_fini(fsname);

out:

	libzfs_close(zhp);

    return error;
}
