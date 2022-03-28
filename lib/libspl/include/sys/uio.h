/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*
 * University Copyright- Copyright (c) 1982, 1986, 1988
 * The Regents of the University of California
 * All Rights Reserved
 *
 * University Acknowledgment- Portions of this document are derived from
 * software developed by the University of California, Berkeley, and its
 * contributors.
 */

#ifndef	_LIBSPL_SYS_UIO_H
#define	_LIBSPL_SYS_UIO_H

#include <sys/debug.h>
#include <sys/types.h>
#include_next <sys/uio.h>

#ifdef __APPLE__
#include <sys/_types/_iovec_t.h>
#endif

#include <stdint.h>
typedef struct iovec iovec_t;

#if defined(__linux__) || defined(__APPLE__)
typedef enum zfs_uio_rw {
	UIO_READ =	0,
	UIO_WRITE =	1,
} zfs_uio_rw_t;

typedef enum zfs_uio_seg {
	UIO_USERSPACE =	0,
	UIO_SYSSPACE =	1,
} zfs_uio_seg_t;

#elif defined(__FreeBSD__)
typedef enum uio_seg  zfs_uio_seg_t;
#endif

typedef struct zfs_uio {
	const struct iovec	*uio_iov;	/* pointer to array of iovecs */
	int		uio_iovcnt;	/* number of iovecs */
	offset_t	uio_loffset;	/* file offset */
	zfs_uio_seg_t	uio_segflg;	/* address space (kernel or user) */
	uint16_t	uio_fmode;	/* file mode flags */
	uint16_t	uio_extflg;	/* extended flags */
	ssize_t		uio_resid;	/* residual count */
	size_t		uio_skip;
} zfs_uio_t;

#define	zfs_uio_segflg(uio)		(uio)->uio_segflg
#define	zfs_uio_offset(uio)		(uio)->uio_loffset
#define	zfs_uio_resid(uio)		(uio)->uio_resid
#define	zfs_uio_iovcnt(uio)		(uio)->uio_iovcnt
#define	zfs_uio_iovlen(uio, idx)	(uio)->uio_iov[(idx)].iov_len
#define	zfs_uio_iovbase(uio, idx)	(uio)->uio_iov[(idx)].iov_base

static inline void
zfs_uio_advance(zfs_uio_t *uio, size_t size)
{
	uio->uio_resid -= size;
	uio->uio_loffset += size;
}

static inline void
zfs_uio_setoffset(zfs_uio_t *uio, offset_t off)
{
	uio->uio_loffset = off;
}

static inline void
zfs_uio_iovec_init(zfs_uio_t *uio, const struct iovec *iov,
    unsigned long nr_segs, offset_t offset, zfs_uio_seg_t seg, ssize_t resid,
    size_t skip)
{
	ASSERT(seg == UIO_USERSPACE);

	uio->uio_iov = iov;
	uio->uio_iovcnt = nr_segs;
	uio->uio_loffset = offset;
	uio->uio_segflg = seg;
	uio->uio_fmode = 0;
	uio->uio_extflg = 0;
	uio->uio_resid = resid;
	uio->uio_skip = skip;
}

#endif	/* _SYS_UIO_H */
