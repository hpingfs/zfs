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
#include <libzfs.h>
#include <locale.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/zfs_context.h>
#include <sys/uio.h>
#include <libuzfs.h>

libzfs_handle_t *g_zfs;

int libuzfs_stat(const char *path, struct stat *statbuf)
{
	zfs_handle_t *zhp;
    int ret = 0;
	int types = ZFS_TYPE_FILESYSTEM;

    char fsname[256] = "";
    char target_path[256] = "";

    char *fs_end = strstr(path, "://");
    memcpy(fsname, path, fs_end - path);
    memcpy(target_path, fs_end + 3, strlen(path) - strlen(fsname) - 3);

    printf("stat %s: %s\n", fsname, target_path);

	if ((g_zfs = libzfs_init()) == NULL) {
		(void) fprintf(stderr, "%s\n", libzfs_error_init(errno));
		return (1);
	}

	libzfs_print_on_error(g_zfs, B_TRUE);


	if ((zhp = libzfs_open(g_zfs, fsname, types)) == NULL)
		return (1);

    ret = uzfs_stat(fsname, target_path, statbuf);

	libzfs_close(zhp);

	libzfs_fini(g_zfs);

	/*
	 * The 'ZFS_ABORT' environment variable causes us to dump core on exit
	 * for the purposes of running ::findleaks.
	 */
	if (getenv("ZFS_ABORT") != NULL) {
		(void) printf("dumping core by request\n");
		abort();
	}


	return (ret);
}

#if 0
int libuzfs_read(const char* path, char *buf, size_t size)
{
	zfs_handle_t *zhp;
    int ret = 0;
	int types = ZFS_TYPE_FILESYSTEM;

    char fsname[256] = "";
    char target_path[256] = "";

    char *fs_end = strstr(path, "://");
    memcpy(fsname, path, fs_end - path);
    memcpy(target_path, fs_end + 3, strlen(path) - strlen(fsname) - 3);

    printf("stat %s: %s\n", fsname, target_path);

	if ((g_zfs = libzfs_init()) == NULL) {
		(void) fprintf(stderr, "%s\n", libzfs_error_init(errno));
		return (1);
	}

	libzfs_print_on_error(g_zfs, B_TRUE);

	if ((zhp = libzfs_open(g_zfs, fsname, types)) == NULL)
		return (1);

    ret = uzfs_init(fsname);
    if (ret) goto out;

    uint64_t root_inode = 0;

    ret = uzfs_getroot(fsname, &root_ino);
    if (ret) goto out;

    char *s = target_path;
    char *e = strchr(s, '/');
    uint64_t dino = root_ino;
    uint64_t ino = 0;

    while (s) {
        if (e)
            *e = '\0';
        else
            out_flag = true;

        ret = uzfs_lookup(dino, s, &ino);
        if (ret) goto out;

        if (out_flag)
            break;

        s = e + 1;
        e = strchr(s, '/');
        dino = ino;
    }

    if (ino == 0) {
        printf("Empty path\n");
        ret = 1;
        goto out;
    }

    struct iovec iov;
    iov.iov_base = buf;
    iov.iov_len = size;

    zfs_uio_t uio;
    zfs_uio_iovec_init(&uio, iov, 1, 0, UIO_USERSPACE, iov->iov_len, 0);

    ret = uzfs_read(ino, &uio, 0);

out:

	libzfs_close(zhp);

	libzfs_fini(g_zfs);

	/*
	 * The 'ZFS_ABORT' environment variable causes us to dump core on exit
	 * for the purposes of running ::findleaks.
	 */
	if (getenv("ZFS_ABORT") != NULL) {
		(void) printf("dumping core by request\n");
		abort();
	}


	return (ret);

}
#endif
