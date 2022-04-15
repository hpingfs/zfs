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
 * Copyright (c) 2012, 2018 by Delphix. All rights reserved.  * Copyright (c) 2016 Actifio, Inc. All rights reserved.
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
#include <sys/zstd/zstd.h>
#include <sys/zvol.h>
#include <zfs_fletcher.h>
#include <zlib.h>

unsigned long totalram_pages = 0x800000 ;

int ddi_copyin(const void *from, void *to, size_t len, int flags)
{
    memcpy(to, from, len);
    return 0;
}

int ddi_copyout(const void *from, void *to, size_t len, int flags)
{
    memcpy(to, from, len);
    return 0;
}

int xcopyin(const void *from, void *to, size_t len)
{
    memcpy(to, from, len);
    return 0;
}

int xcopyout(const void *from, void *to, size_t len)
{
    memcpy(to, from, len);
    return 0;
}

int copyinstr(const void *from, void *to, size_t len, size_t *done)
{
	if (len == 0)
		return (-ENAMETOOLONG);

	/* XXX: Should return ENAMETOOLONG if 'strlen(from) > len' */

	memset(to, 0, len);
    if (xcopyin(from, to, len - 1) == 0) {
        if (done) {
            *done = 0;
        }
    } else {
        if (done) {
            *done = len - 1;
        }
    }

	return (0);
}

boolean_t zfs_proc_is_caller(struct task_struct *t)
{
	return B_FALSE;
}


