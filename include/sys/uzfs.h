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

#ifndef	_SYS_UZFS_H
#define	_SYS_UZFS_H

#include <sys/zfs_context.h>

#define zpl_super_operations uzfs_super_operations
#define zpl_export_operations uzfs_export_operations
#define zpl_dentry_operations uzfs_dentry_operations
#define zpl_xattr_handlers uzfs_xattr_handlers

extern void zpl_prune_sb(int64_t nr_to_scan, void *arg);

#define zfs_uio_fault_disable(u, set)

#if defined(HAVE_INODE_OWNER_OR_CAPABLE)
#define zpl_inode_owner_or_capable(ns, ip)  inode_owner_or_capable(ip)
#elif defined(HAVE_INODE_OWNER_OR_CAPABLE_IDMAPPED)
#define zpl_inode_owner_or_capable(ns, ip)  inode_owner_or_capable(ns, ip)
#else
#error "Unsupported kernel"
#endif

#endif	/* _SYS_UZFS_H */
