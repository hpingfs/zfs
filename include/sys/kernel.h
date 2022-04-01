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

#ifndef	_SYS_KERNEL_H
#define	_SYS_KERNEL_H

#include <sys/zfs_context.h>

struct spinlock_t;

extern struct inode *igrab(struct inode *inode);
extern void iput(struct inode *inode);
extern void drop_nlink(struct inode *inode);
extern void clear_nlink(struct inode *inode);
extern void set_nlink(struct inode *inode, unsigned int nlink);
extern void inc_nlink(struct inode *inode);

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:    the pointer to the member.
 * @type:   the type of the container struct this is embedded in.
 * @member: the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({          \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type,member) );})

typedef struct {
            int counter;
} atomic_t;

extern int atomic_read(const atomic_t *v);
#define atomic_inc_not_zero(v)      atomic_add_unless((v), 1, 0)

typedef	int	umode_t;

typedef unsigned int kuid_t;
typedef unsigned int kgid_t;

#define make_kuid(ns, uid) KUIDT_INIT(uid)
#define make_kgid(ns, gid) KGIDT_INIT(gid)

#define KUIDT_INIT(value) ((kuid_t) value )
#define KGIDT_INIT(value) ((kgid_t) value )

static inline kuid_t __kuid_val(kuid_t uid)
{
    return uid;
}

static inline kgid_t __kgid_val(kgid_t gid)
{
    return gid;
}

extern struct timespec timespec_trunc(struct timespec t, unsigned gran);
extern struct timespec current_kernel_time(void);

#if defined(CONFIG_64BIT)
#define	TIME_MAX			INT64_MAX
#define	TIME_MIN			INT64_MIN
#else
#define	TIME_MAX			INT32_MAX
#define	TIME_MIN			INT32_MIN
#endif

#define	TIMESPEC_OVERFLOW(ts)		\
	((ts)->tv_sec < TIME_MIN || (ts)->tv_sec > TIME_MAX)


#define task_io_account_read(n)
#define task_io_account_write(n)


/*
 * 4.9 API change
 *  Preferred interface to get the current FS time.
 * */
#if !defined(HAVE_CURRENT_TIME)
extern struct timespec current_time(struct inode *ip);
#endif

#define S_IRWXUGO   (S_IRWXU|S_IRWXG|S_IRWXO)

// FIXME(hping) from os/linux/kernel/linux/vfs_compat.h
/*
 * 4.14 adds SB_* flag definitions, define them to MS_* equivalents
 * if not set.
 */
#ifndef SB_RDONLY
#define SB_RDONLY   MS_RDONLY
#endif

#ifndef SB_SILENT
#define SB_SILENT   MS_SILENT
#endif

#ifndef SB_ACTIVE
#define SB_ACTIVE   MS_ACTIVE
#endif

#ifndef SB_POSIXACL
#define SB_POSIXACL MS_POSIXACL
#endif

#ifndef SB_MANDLOCK
#define SB_MANDLOCK MS_MANDLOCK
#endif

#ifndef SB_NOATIME
#define SB_NOATIME  MS_NOATIME
#endif

/*
 * Inode flags - they have no relation to superblock flags now
 */
#define S_SYNC      1   /* Writes are synced at once */
#define S_NOATIME   2   /* Do not update access times */
#define S_APPEND    4   /* Append-only file */
#define S_IMMUTABLE 8   /* Immutable file */
#define S_DEAD      16  /* removed, but still open directory */
#define S_NOQUOTA   32  /* Inode is not counted to quota */
#define S_DIRSYNC   64  /* Directory modifications are synchronous */
#define S_NOCMTIME  128 /* Do not update file c/mtime */
#define S_SWAPFILE  256 /* Do not truncate: swapon got its bmaps */
#define S_PRIVATE   512 /* Inode is fs-internal */
#define S_IMA       1024    /* Inode has an associated IMA struct */
#define S_AUTOMOUNT 2048    /* Automount/referral quasi-directory */
#define S_NOSEC     4096    /* no suid or xattr security attributes */
#define S_IOPS_WRAPPER  8192    /* i_op points to struct inode_operations_wrapper */

#define printk(...) ((void) 0)

#define mappedread(a,b,c) fake_mappedread(a,b,c)

struct cred {
//	atomic_t	usage;
//#ifdef CONFIG_DEBUG_CREDENTIALS
//	atomic_t	subscribers;	/* number of processes subscribed */
//	void		*put_addr;
//	unsigned	magic;
//#define CRED_MAGIC	0x43736564
//#define CRED_MAGIC_DEAD	0x44656144
//#endif
//	kuid_t		uid;		/* real UID of the task */
//	kgid_t		gid;		/* real GID of the task */
//	kuid_t		suid;		/* saved UID of the task */
//	kgid_t		sgid;		/* saved GID of the task */
//	kuid_t		euid;		/* effective UID of the task */
//	kgid_t		egid;		/* effective GID of the task */
//	kuid_t		fsuid;		/* UID for VFS ops */
//	kgid_t		fsgid;		/* GID for VFS ops */
//	unsigned	securebits;	/* SUID-less security management */
//	kernel_cap_t	cap_inheritable; /* caps our children can inherit */
//	kernel_cap_t	cap_permitted;	/* caps we're permitted */
//	kernel_cap_t	cap_effective;	/* caps we can actually use */
//	kernel_cap_t	cap_bset;	/* capability bounding set */
//#ifdef CONFIG_KEYS
//	unsigned char	jit_keyring;	/* default keyring to attach requested
//					 * keys to */
//	struct key __rcu *session_keyring; /* keyring inherited over fork */
//	struct key	*process_keyring; /* keyring private to this process */
//	struct key	*thread_keyring; /* keyring private to this thread */
//	struct key	*request_key_auth; /* assumed request_key authority */
//#endif
//#ifdef CONFIG_SECURITY
//	void		*security;	/* subjective LSM security */
//#endif
//	struct user_struct *user;	/* real user ID subscription */
//	struct user_namespace *user_ns; /* user_ns the caps and keyrings are relative to. */
//	struct group_info *group_info;	/* supplementary groups for euid/fsgid */
//	struct rcu_head	rcu;		/* RCU deletion hook */
//
//	RH_KABI_EXTEND(kernel_cap_t cap_ambient)  /* Ambient capability set */
};

/* Page cache limit. The filesystems should put that into their s_maxbytes
   limits, otherwise bad things can happen in VM. */
//#if BITS_PER_LONG==32
//#define MAX_LFS_FILESIZE    (((loff_t)PAGE_CACHE_SIZE << (BITS_PER_LONG-1))-1)
//#elif BITS_PER_LONG==64
#define MAX_LFS_FILESIZE    ((loff_t)0x7fffffffffffffffLL)
//#endif

#define u32 __u32
typedef unsigned int __u32;
struct inode_operations;
struct file_operations;

struct super_operations {};
struct export_operations {};
struct xattr_handler {};
struct dentry_operations {};

struct backing_dev_info {
//	struct list_head bdi_list;
	unsigned long ra_pages;	/* max readahead in PAGE_CACHE_SIZE units */
//	unsigned long state;	/* Always use atomic bitops on this */
//	unsigned int capabilities; /* Device capabilities */
//	congested_fn *congested_fn; /* Function pointer if device is md/dm */
//	void *congested_data;	/* Pointer to aux data for congested func */
//
//	char *name;
//
//	struct percpu_counter bdi_stat[NR_BDI_STAT_ITEMS];
//
//	unsigned long bw_time_stamp;	/* last time write bw is updated */
//	unsigned long dirtied_stamp;
//	unsigned long written_stamp;	/* pages written at bw_time_stamp */
//	unsigned long write_bandwidth;	/* the estimated write bandwidth */
//	unsigned long avg_write_bandwidth; /* further smoothed write bw */
//
//	/*
//	 * The base dirty throttle rate, re-calculated on every 200ms.
//	 * All the bdi tasks' dirty rate will be curbed under it.
//	 * @dirty_ratelimit tracks the estimated @balanced_dirty_ratelimit
//	 * in small steps and is much more smooth/stable than the latter.
//	 */
//	unsigned long dirty_ratelimit;
//	unsigned long balanced_dirty_ratelimit;
//
//	struct fprop_local_percpu completions;
//	int dirty_exceeded;
//
//	unsigned int min_ratio;
//	unsigned int max_ratio, max_prop_frac;
//
//	struct bdi_writeback wb;  /* default writeback info for this bdi */
//	spinlock_t wb_lock;	  /* protects work_list & wb.dwork scheduling */ //
//	struct list_head work_list;
//
//	struct device *dev;
//
//	struct timer_list laptop_mode_wb_timer;
//
//#ifdef CONFIG_DEBUG_FS
//	struct dentry *debug_dir;
//	struct dentry *debug_stats;
//#endif
};


struct super_block {
//	struct list_head	s_list;		/* Keep this first */
//	dev_t			s_dev;		/* search index; _not_ kdev_t */
	unsigned char		s_blocksize_bits;
	unsigned long		s_blocksize;
	loff_t			s_maxbytes;	/* Max file size */
//	struct file_system_type	*s_type;
	const struct super_operations	*s_op;
//	const struct dquot_operations	*dq_op;
//	const struct quotactl_ops	*s_qcop;
	const struct export_operations *s_export_op;
	unsigned long		s_flags;
	unsigned long		s_magic;
	struct dentry		*s_root;
//	struct rw_semaphore	s_umount;
//	int			s_count;
	atomic_t		s_active;
//#ifdef CONFIG_SECURITY
//	void                    *s_security;
//#endif
	const struct xattr_handler **s_xattr;
//
//	struct list_head	s_inodes;	/* all inodes */
//	struct hlist_bl_head	s_anon;		/* anonymous dentries for (nfs) exporting */
//#ifdef __GENKSYMS__
//#ifdef CONFIG_SMP
//	struct list_head __percpu *s_files;
//#else
//	struct list_head	s_files;
//#endif
//#else
//#ifdef CONFIG_SMP
//	struct list_head __percpu *s_files_deprecated;
//#else
//	struct list_head	s_files_deprecated;
//#endif
//#endif
//	struct list_head	s_mounts;	/* list of mounts; _not_ for fs use */
//	/* s_dentry_lru, s_nr_dentry_unused protected by dcache.c lru locks */
//	struct list_head	s_dentry_lru;	/* unused dentry lru */
//	RH_KABI_REPLACE_UNSAFE(
//			int	s_nr_dentry_unused,
//			long	s_nr_dentry_unused)	/* # of dentry on lru */
//
//	/* s_inode_lru_lock protects s_inode_lru and s_nr_inodes_unused */
//	spinlock_t		s_inode_lru_lock ____cacheline_aligned_in_smp;
//	struct list_head	s_inode_lru;		/* unused inode lru */
//	RH_KABI_REPLACE_UNSAFE(
//			int	s_nr_inodes_unused,
//			long	s_nr_inodes_unused)	/* # of inodes on lru */
//
//	struct block_device	*s_bdev;
	struct backing_dev_info *s_bdi;
//	struct mtd_info		*s_mtd;
//	struct hlist_node	s_instances;
//	struct quota_info	s_dquot;	/* Diskquota specific options */
//
//	struct sb_writers	s_writers;
//
//	char s_id[32];				/* Informational name */
//	u8 s_uuid[16];				/* UUID */
//
	void 			*s_fs_info;	/* Filesystem private info */
//	unsigned int		s_max_links;
//	fmode_t			s_mode;
//
//	/* Granularity of c/m/atime in ns.
//	   Cannot be worse than a second */
	u32		   s_time_gran;
//
//	/*
//	 * The next field is for VFS *only*. No filesystems have any business
//	 * even looking at it. You had been warned.
//	 */
//	struct mutex s_vfs_rename_mutex;	/* Kludge */
//
//	/*
//	 * Filesystem subtype.  If non-empty the filesystem type field
//	 * in /proc/mounts will be "type.subtype"
//	 */
//	char *s_subtype;
//
//	/*
//	 * Saved mount options for lazy filesystems using
//	 * generic_show_options()
//	 */
//	char __rcu *s_options;
	const struct dentry_operations *s_d_op; /* default d_op for dentries */
//
//	/*
//	 * Saved pool identifier for cleancache (-1 means none)
//	 */
//	int cleancache_poolid;
//
//	struct shrinker s_shrink;	/* per-sb shrinker handle */
//
//	/* Number of inodes with nlink == 0 but still referenced */
//	atomic_long_t s_remove_count;
//
//	/* Being remounted read-only */
//	int s_readonly_remount;
//
//	/* AIO completions deferred from interrupt context */
//	RH_KABI_EXTEND(struct workqueue_struct *s_dio_done_wq)
//	RH_KABI_EXTEND(struct rcu_head rcu)
//	RH_KABI_EXTEND(struct hlist_head s_pins)
//
//	/* s_inode_list_lock protects s_inodes */
//	RH_KABI_EXTEND(spinlock_t s_inode_list_lock)
//
//	RH_KABI_EXTEND(struct mutex s_sync_lock) /* sync serialisation lock */
//
//	RH_KABI_EXTEND(spinlock_t s_inode_wblist_lock)
//	RH_KABI_EXTEND(struct list_head s_inodes_wb)	/* writeback inodes */
//
//	RH_KABI_EXTEND(unsigned long	s_iflags)
//	RH_KABI_EXTEND(struct user_namespace *s_user_ns)
//
//	/* Pending fsnotify inode refs */
//	RH_KABI_EXTEND(atomic_long_t s_fsnotify_inode_refs)
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

const struct super_operations uzfs_super_operations;
const struct export_operations uzfs_export_operations;
const struct dentry_operations uzfs_dentry_operations;
const struct xattr_handler *uzfs_xattr_handlers;



#define register_filesystem(f)
#define unregister_filesystem(f)

#define deactivate_super(s)

#define jiffies 0
#define time_after(m,n) (0)

extern long zfsdev_ioctl(unsigned cmd, unsigned long arg);

#endif	/* _SYS_KERNEL_H */
