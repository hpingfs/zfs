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
 * Copyright 2016 Nexenta Systems, Inc. All rights reserved.
 */

#ifndef	_SYS_ZFS_ZNODE_IMPL_H
#define	_SYS_ZFS_ZNODE_IMPL_H

#include <sys/isa_defs.h>
#include <sys/types32.h>
#include <sys/list.h>
#include <sys/stat.h>
#include <sys/dmu.h>
#include <sys/sa.h>
#include <sys/time.h>
#include <sys/zfs_vfsops.h>
#include <sys/rrwlock.h>
#include <sys/zfs_sa.h>
#include <sys/zfs_stat.h>
#include <sys/zfs_rlock.h>

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef _KERNEL

typedef unsigned int __u32;
struct inode_operations;
struct file_operations;

struct super_block {
	void 			*s_fs_info;	/* Filesystem private info */
	unsigned long		s_flags;
	unsigned int    s_time_gran;
};

/*
 * Keep mostly read-only and often accessed (especially for
 * the RCU path lookup and 'stat' data) fields at the beginning
 * of the 'struct inode'
 */
struct inode {
	umode_t			i_mode;
	unsigned short		i_opflags;
	kuid_t			i_uid;
	kgid_t			i_gid;
	unsigned int		i_flags;

//#ifdef CONFIG_FS_POSIX_ACL
//	struct posix_acl	*i_acl;
//	struct posix_acl	*i_default_acl;
//#endif

	const struct inode_operations	*i_op;
	struct super_block	*i_sb;
//	struct address_space	*i_mapping;

//#ifdef CONFIG_SECURITY
//	void			*i_security;
//#endif

	/* Stat data, not accessed from path walking */
	unsigned long		i_ino;
	/*
	 * Filesystems may only read i_nlink directly.  They shall use the
	 * following functions for modification:
	 *
	 *    (set|clear|inc|drop)_nlink
	 *    inode_(inc|dec)_link_count
	 */
	union {
		const unsigned int i_nlink;
		unsigned int __i_nlink;
	};
//	dev_t			i_rdev;
	loff_t			i_size;
	struct timespec		i_atime;
	struct timespec		i_mtime;
	struct timespec		i_ctime;
	spinlock_t		i_lock;	/* i_blocks, i_bytes, maybe i_size */
	unsigned short          i_bytes;
	unsigned int		i_blkbits;
//#if defined(CONFIG_IMA) && (defined(CONFIG_PPC64) || defined(CONFIG_S390))
//	/* 4 bytes hole available on both required architectures */
//	RH_KABI_FILL_HOLE(atomic_t		i_readcount)
//#endif
	blkcnt_t		i_blocks;
//
//#ifdef __NEED_I_SIZE_ORDERED
//	seqcount_t		i_size_seqcount;
//#endif

	/* Misc */
	unsigned long		i_state;
//	struct kmutex		i_mutex;

	unsigned long		dirtied_when;	/* jiffies of first dirtying */

//	struct hlist_node	i_hash;
//	RH_KABI_RENAME(struct list_head i_wb_list,
//		       struct list_head	i_io_list); /* backing dev IO list */
//	struct list_head	i_lru;		/* inode LRU list */
//	struct list_head	i_sb_list;
//	union {
//		struct hlist_head	i_dentry;
//		struct rcu_head		i_rcu;
//	};
//	__u64			i_version;
	atomic_t		i_count;
	atomic_t		i_dio_count;
	atomic_t		i_writecount;
	const struct file_operations	*i_fop;	/* former ->i_op->default_file_ops */
//	struct file_lock	*i_flock;
//	struct address_space	i_data;
//#ifdef CONFIG_QUOTA
//	struct dquot		*i_dquot[MAXQUOTAS];
//#endif
//	struct list_head	i_devices;
//	union {
//		struct pipe_inode_info	*i_pipe;
//		struct block_device	*i_bdev;
//		struct cdev		*i_cdev;
//	};
	__u32			i_generation;
//
//#ifdef CONFIG_FSNOTIFY
//	__u32			i_fsnotify_mask; /* all events this inode cares about */
//	RH_KABI_REPLACE(struct hlist_head i_fsnotify_marks,
//			struct fsnotify_mark_connector __rcu *i_fsnotify_marks)
//#endif
//
//#if defined(CONFIG_IMA) && defined(CONFIG_X86_64)
//	atomic_t		i_readcount; /* struct files open RO */
//#endif
	void			*i_private; /* fs or device private pointer */
};

#endif

#define	ZNODE_OS_FIELDS			\
	inode_timespec_t z_btime; /* creation/birth time (cached) */ \
	struct inode	z_inode;

/*
 * Convert between znode pointers and inode pointers
 */
#define	ZTOI(znode)	(&((znode)->z_inode))
#define	ITOZ(inode)	(container_of((inode), znode_t, z_inode))
#define	ZTOZSB(znode)	((zfsvfs_t *)(ZTOI(znode)->i_sb->s_fs_info))
#define	ITOZSB(inode)	((zfsvfs_t *)((inode)->i_sb->s_fs_info))

#define	ZTOTYPE(zp)	(ZTOI(zp)->i_mode)
#define	ZTOGID(zp) (ZTOI(zp)->i_gid)
#define	ZTOUID(zp) (ZTOI(zp)->i_uid)
#define	ZTONLNK(zp) (ZTOI(zp)->i_nlink)

#define	Z_ISBLK(type) S_ISBLK(type)
#define	Z_ISCHR(type) S_ISCHR(type)
#define	Z_ISLNK(type) S_ISLNK(type)
#define	Z_ISDEV(type)	(S_ISCHR(type) || S_ISBLK(type) || S_ISFIFO(type))
#define	Z_ISDIR(type)	S_ISDIR(type)

#define	zn_has_cached_data(zp)		((zp)->z_is_mapped)
#define	zn_flush_cached_data(zp, sync)	write_inode_now(ZTOI(zp), sync)
#define	zn_rlimit_fsize(zp, uio)	(0)

/*
 * zhold() wraps igrab() on Linux, and igrab() may fail when the
 * inode is in the process of being deleted.  As zhold() must only be
 * called when a ref already exists - so the inode cannot be
 * mid-deletion - we VERIFY() this.
 */
#define	zhold(zp)	VERIFY3P(igrab(ZTOI((zp))), !=, NULL)
#define	zrele(zp)	iput(ZTOI((zp)))

/* Called on entry to each ZFS inode and vfs operation. */
#define	ZFS_ENTER_ERROR(zfsvfs, error)				\
do {								\
	ZFS_TEARDOWN_ENTER_READ(zfsvfs, FTAG);			\
	if (unlikely((zfsvfs)->z_unmounted)) {			\
		ZFS_TEARDOWN_EXIT_READ(zfsvfs, FTAG);		\
		return (error);					\
	}							\
} while (0)
#define	ZFS_ENTER(zfsvfs)	ZFS_ENTER_ERROR(zfsvfs, EIO)
#define	ZPL_ENTER(zfsvfs)	ZFS_ENTER_ERROR(zfsvfs, -EIO)

/* Must be called before exiting the operation. */
#define	ZFS_EXIT(zfsvfs)					\
do {								\
	zfs_exit_fs(zfsvfs);					\
	ZFS_TEARDOWN_EXIT_READ(zfsvfs, FTAG);			\
} while (0)

#define	ZPL_EXIT(zfsvfs)					\
do {								\
	rrm_exit(&(zfsvfs)->z_teardown_lock, FTAG);		\
} while (0)

/* Verifies the znode is valid. */
#define	ZFS_VERIFY_ZP_ERROR(zp, error)				\
do {								\
	if (unlikely((zp)->z_sa_hdl == NULL)) {			\
		ZFS_EXIT(ZTOZSB(zp));				\
		return (error);					\
	}							\
} while (0)
#define	ZFS_VERIFY_ZP(zp)	ZFS_VERIFY_ZP_ERROR(zp, EIO)
#define	ZPL_VERIFY_ZP(zp)	ZFS_VERIFY_ZP_ERROR(zp, -EIO)

/*
 * Macros for dealing with dmu_buf_hold
 */
#define	ZFS_OBJ_MTX_SZ		64
#define	ZFS_OBJ_MTX_MAX		(1024 * 1024)
#define	ZFS_OBJ_HASH(zfsvfs, obj)	((obj) & ((zfsvfs->z_hold_size) - 1))

extern unsigned int zfs_object_mutex_size;

/*
 * Encode ZFS stored time values from a struct timespec / struct timespec64.
 */
#define	ZFS_TIME_ENCODE(tp, stmp)		\
do {						\
	(stmp)[0] = (uint64_t)(tp)->tv_sec;	\
	(stmp)[1] = (uint64_t)(tp)->tv_nsec;	\
} while (0)

#if defined(HAVE_INODE_TIMESPEC64_TIMES)
/*
 * Decode ZFS stored time values to a struct timespec64
 * 4.18 and newer kernels.
 */
#define	ZFS_TIME_DECODE(tp, stmp)		\
do {						\
	(tp)->tv_sec = (time64_t)(stmp)[0];	\
	(tp)->tv_nsec = (long)(stmp)[1];	\
} while (0)
#else
/*
 * Decode ZFS stored time values to a struct timespec
 * 4.17 and older kernels.
 */
#define	ZFS_TIME_DECODE(tp, stmp)		\
do {						\
	(tp)->tv_sec = (time_t)(stmp)[0];	\
	(tp)->tv_nsec = (long)(stmp)[1];	\
} while (0)
#endif /* HAVE_INODE_TIMESPEC64_TIMES */

#define	ZFS_ACCESSTIME_STAMP(zfsvfs, zp)

struct znode;

extern int	zfs_sync(struct super_block *, int, cred_t *);
extern int	zfs_inode_alloc(struct super_block *, struct inode **ip);
extern void	zfs_inode_destroy(struct inode *);
extern void	zfs_mark_inode_dirty(struct inode *);
extern boolean_t zfs_relatime_need_update(const struct inode *);

#if defined(HAVE_UIO_RW)
extern caddr_t zfs_map_page(page_t *, enum seg_rw);
extern void zfs_unmap_page(page_t *, caddr_t);
#endif /* HAVE_UIO_RW */

extern zil_replay_func_t *const zfs_replay_vector[TX_MAX_TYPE];

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_ZFS_ZNODE_IMPL_H */
