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
#include <sys/uspl.h>
#include <sys/mount.h>
#include <sys/gfp.h>
#include <sys/vnode.h>

#define WARN_ON(s) ASSERT(!(s))

struct spinlock_t;

#define IS_ERR(ptr) (B_FALSE)

#define LOOKUP_FOLLOW       0x0001
#define LOOKUP_DIRECTORY    0x0002

#define u32 __u32
typedef unsigned int __u32;
typedef unsigned long int u64;

typedef struct znode znode_t;
typedef struct objset objset_t;
typedef struct zfs_cmd zfs_cmd_t;

struct path;
struct file;
struct inode;

typedef struct filldir {} filldir_t;
typedef struct zpl_dir_context {
    void *dirent;
    const filldir_t actor;
    loff_t pos;
} zpl_dir_context_t;

#define ZPL_DIR_CONTEXT_INIT(_dirent, _actor, _pos) {   \
    .pos = _pos,                    \
}

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

struct linux_kstat {
    u64     ino;
    dev_t       dev;
    umode_t     mode;
    unsigned int    nlink;
    kuid_t      uid;
    kgid_t      gid;
    dev_t       rdev;
    loff_t      size;
    struct timespec  atime;
    struct timespec mtime;
    struct timespec ctime;
    unsigned long   blksize;
    unsigned long long  blocks;
};

struct user_namespace {};
struct file_system_type {};
struct super_operations {};
struct export_operations {};
struct xattr_handler {};


struct dentry_operations {
    int (*d_revalidate)(struct dentry *, unsigned int);
//    int (*d_weak_revalidate)(struct dentry *, unsigned int);
//    int (*d_hash)(const struct dentry *, struct qstr *);
//    int (*d_compare)(const struct dentry *, const struct dentry *,
//            unsigned int, const char *, const struct qstr *);
//    int (*d_delete)(const struct dentry *);
//    void (*d_release)(struct dentry *);
//    void (*d_prune)(struct dentry *);
//    void (*d_iput)(struct dentry *, struct inode *);
//    char *(*d_dname)(struct dentry *, char *, int);
    struct vfsmount *(*d_automount)(struct path *);
//    RH_KABI_REPLACE(int (*d_manage)(struct dentry *, bool),
//            int (*d_manage)(const struct path *, bool))
};
typedef const struct dentry_operations	dentry_operations_t;

struct file_operations {
//    struct module *owner;
    loff_t (*llseek) (struct file *, loff_t, int);
    ssize_t (*read) (struct file *, char *, size_t, loff_t *);
//    ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
//    ssize_t (*aio_read) (struct kiocb *, const struct iovec *, unsigned long, loff_t);
//    ssize_t (*aio_write) (struct kiocb *, const struct iovec *, unsigned long, loff_t);
    int (*readdir) (struct file *, void *, filldir_t);
//    unsigned int (*poll) (struct file *, struct poll_table_struct *);
//    long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
//    long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
//    int (*mmap) (struct file *, struct vm_area_struct *);
    int (*open) (struct inode *, struct file *);
//    int (*flush) (struct file *, fl_owner_t id);
//    int (*release) (struct inode *, struct file *);
//    int (*fsync) (struct file *, loff_t, loff_t, int datasync);
//    int (*aio_fsync) (struct kiocb *, int datasync);
//    int (*fasync) (int, struct file *, int);
//    int (*lock) (struct file *, int, struct file_lock *);
//    ssize_t (*sendpage) (struct file *, struct page *, int, size_t, loff_t *, int);
//    unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long, unsigned long, unsigned long);
//    int (*check_flags)(int);
//    int (*flock) (struct file *, int, struct file_lock *);
//    ssize_t (*splice_write)(struct pipe_inode_info *, struct file *, loff_t *, size_t, unsigned int);
//    ssize_t (*splice_read)(struct file *, loff_t *, struct pipe_inode_info *, size_t, unsigned int);
//    RH_KABI_REPLACE(int (*setlease)(struct file *, long, struct file_lock **), int (*setlease)(struct file *, long, struct file_lock **, void **))
//    long (*fallocate)(struct file *file, int mode, loff_t offset,
//              loff_t len);
//    int (*show_fdinfo)(struct seq_file *m, struct file *f);
//    RH_KABI_EXTEND(int (*iterate) (struct file *, struct dir_context *))
};

struct inode_operations {
    struct dentry * (*lookup) (struct inode *,struct dentry *, unsigned int);
//    void * (*follow_link) (struct dentry *, struct nameidata *);
//    int (*permission) (struct inode *, int);
//    struct posix_acl * (*get_acl)(struct inode *, int);
//
//    int (*readlink) (struct dentry *, char __user *,int);
//    void (*put_link) (struct dentry *, struct nameidata *, void *);
//
//    int (*create) (struct inode *,struct dentry *, umode_t, bool);
//    int (*link) (struct dentry *,struct inode *,struct dentry *);
//    int (*unlink) (struct inode *,struct dentry *);
//    int (*symlink) (struct inode *,struct dentry *,const char *);
    int (*mkdir) (struct inode *,struct dentry *,umode_t);
    int (*rmdir) (struct inode *,struct dentry *);
//    int (*mknod) (struct inode *,struct dentry *,umode_t,dev_t);
    int (*rename) (struct inode *, struct dentry *, struct inode *, struct dentry *);
//    int (*setattr) (struct dentry *, struct iattr *);
    int (*getattr) (struct vfsmount *mnt, struct dentry *, struct linux_kstat *);
//    int (*setxattr) (struct dentry *, const char *,const void *,size_t,int);
//    ssize_t (*getxattr) (struct dentry *, const char *, void *, size_t);
//    ssize_t (*listxattr) (struct dentry *, char *, size_t);
//    int (*removexattr) (struct dentry *, const char *);
//    int (*fiemap)(struct inode *, struct fiemap_extent_info *, u64 start,
//              u64 len);
//    int (*update_time)(struct inode *, struct timespec *, int);
//    int (*atomic_open)(struct inode *, struct dentry *,
//               struct file *, unsigned open_flag,
//               umode_t create_mode, int *opened);
};

struct address_space_operations {};

extern struct file_system_type uzfs_fs_type;
extern const struct super_operations uzfs_super_operations;
extern const struct export_operations uzfs_export_operations;
extern const struct dentry_operations uzfs_dentry_operations;
extern const struct inode_operations uzfs_inode_operations;
extern const struct inode_operations uzfs_dir_inode_operations;
extern const struct inode_operations uzfs_symlink_inode_operations;
extern const struct inode_operations uzfs_special_inode_operations;
extern const struct file_operations uzfs_file_operations;
extern const struct file_operations uzfs_dir_file_operations;
extern const struct address_space_operations uzfs_address_space_operations;

// zfs_ctldir
extern const struct file_operations zpl_fops_root;
extern const struct inode_operations zpl_ops_root;
extern const struct file_operations zpl_fops_snapdir;
extern const struct inode_operations zpl_ops_snapdir;
extern const struct file_operations zpl_fops_shares;
extern const struct inode_operations zpl_ops_shares;
extern const struct file_operations simple_dir_operations;
extern const struct inode_operations simple_dir_inode_operations;

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
    volatile uint32_t counter;
} atomic_t;


#if defined(CONFIG_64BIT)
#define	TIME_MAX			INT64_MAX
#define	TIME_MIN			INT64_MIN
#else
#define	TIME_MAX			INT32_MAX
#define	TIME_MIN			INT32_MIN
#endif

#define	TIMESPEC_OVERFLOW(ts)		\
	((ts)->tv_sec < TIME_MIN || (ts)->tv_sec > TIME_MAX)


#define jiffies 0
#define time_after(m,n) (0)

static inline void task_io_account_read(int64_t n) {}
static inline void task_io_account_write(int64_t n) {}

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

/*
 * This struct is used to pass information from page reclaim to the shrinkers.
 * We consolidate the values for easier extention later.
 */
struct shrink_control {
	gfp_t gfp_mask;

	/* How many slab objects shrinker() should scan and try to reclaim */
	unsigned long nr_to_scan;
};

typedef atomic_t atomic_long_t;
/*
 * A callback you can register to apply pressure to ageable caches.
 *
 * 'sc' is passed shrink_control which includes a count 'nr_to_scan'
 * and a 'gfpmask'.  It should look through the least-recently-used
 * 'nr_to_scan' entries and attempt to free them up.  It should return
 * the number of objects which remain in the cache.  If it returns -1, it means
 * it cannot do any scanning at this time (eg. there is a risk of deadlock).
 *
 * The 'gfpmask' refers to the allocation we are currently trying to
 * fulfil.
 *
 * Note that 'shrink' will be passed nr_to_scan == 0 when the VM is
 * querying the cache size, so a fastpath for that case is appropriate.
 */
struct shrinker {
	int (*shrink)(struct shrinker *, struct shrink_control *sc);
	int seeks;	/* seeks to recreate an obj */
	long batch;	/* reclaim batch size, 0 = default */

	/* These are for internal use */
	struct list_node list;
	atomic_long_t nr_in_batch; /* objs pending delete */
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
	struct shrinker s_shrink;	/* per-sb shrinker handle */
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

struct address_space_operations;
struct address_space {
//    struct inode        *host;      /* owner: inode, block_device */
//    struct radix_tree_root  page_tree;  /* radix tree of all pages */
//    spinlock_t      tree_lock;  /* and lock protecting it */
//    RH_KABI_REPLACE(unsigned int i_mmap_writable,
//             atomic_t i_mmap_writable) /* count VM_SHARED mappings */
//    struct rb_root      i_mmap;     /* tree of private and shared mappings */
//    struct list_head    i_mmap_nonlinear;/*list VM_NONLINEAR mappings */
//    struct mutex        i_mmap_mutex;   /* protect tree, count, list */
//    /* Protected by tree_lock together with the radix tree */
//    unsigned long       nrpages;    /* number of total pages */
//    /* number of shadow or DAX exceptional entries */
//    RH_KABI_RENAME(unsigned long nrshadows,
//               unsigned long nrexceptional);
//    pgoff_t         writeback_index;/* writeback starts here */
    const struct address_space_operations *a_ops;   /* methods */
//    unsigned long       flags;      /* error bits/gfp mask */
//    struct backing_dev_info *backing_dev_info; /* device readahead, etc */
//    spinlock_t      private_lock;   /* for use by the address_space */
//    struct list_head    private_list;   /* ditto */
//    void            *private_data;  /* ditto */
} __attribute__((aligned(sizeof(long))));

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
	struct address_space	*i_mapping;

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
	uint64_t		i_version;
	atomic_t		i_count;
	atomic_t		i_dio_count;
	atomic_t		i_writecount;
	const struct file_operations	*i_fop;	/* former ->i_op->default_file_ops */
//	struct file_lock	*i_flock;
	struct address_space	i_data;
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


#define DCACHE_MOUNTED      0x10000 /* is a mountpoint */
#define DCACHE_NEED_AUTOMOUNT   0x20000 /* handle automount on this dir */
#define DCACHE_MANAGE_TRANSIT   0x40000 /* manage transit from this dirent */
#define DCACHE_MANAGED_DENTRY \
    (DCACHE_MOUNTED|DCACHE_NEED_AUTOMOUNT|DCACHE_MANAGE_TRANSIT)

/* The hash is always the low bits of hash_len */
#ifdef __LITTLE_ENDIAN
 #define HASH_LEN_DECLARE u32 hash; u32 len;
 #define bytemask_from_count(cnt)   (~(~0ul << (cnt)*8))
#else
 #define HASH_LEN_DECLARE u32 len; u32 hash;
 #define bytemask_from_count(cnt)   (~(~0ul >> (cnt)*8))
#endif

/*
 * "quick string" -- eases parameter passing, but more importantly
 * saves "metadata" about the string (ie length and the hash).
 *
 * hash comes first so it snuggles against d_parent in the
 * dentry.
 */
struct qstr {
    union {
        struct {
            HASH_LEN_DECLARE;
        };
        u64 hash_len;
    };
    const unsigned char *name;
};

struct dentry {
//    /* RCU lookup touched fields */
    unsigned int d_flags;       /* protected by d_lock */
//    seqcount_t d_seq;       /* per dentry seqlock */
//    struct hlist_bl_node d_hash;    /* lookup hash list */
//    struct dentry *d_parent;    /* parent directory */
    struct qstr d_name;
    struct inode *d_inode;      /* Where the name belongs to - NULL is
//                     * negative */
//    unsigned char d_iname[DNAME_INLINE_LEN];    /* small names */
//
//    /* Ref lookup also touches following */
//    struct lockref d_lockref;   /* per-dentry lock and refcount */
    const struct dentry_operations *d_op;
//    struct super_block *d_sb;   /* The root of the dentry tree */
//    unsigned long d_time;       /* used by d_revalidate */
//    void *d_fsdata;         /* fs-specific data */
//
//    struct list_head d_lru;     /* LRU list */
//    /*
//     * d_child and d_rcu can share memory
//     */
//    union {
//        struct list_head d_child;   /* child of parent list */
//        struct rcu_head d_rcu;
//    } d_u;
//    struct list_head d_subdirs; /* our children */
//    struct hlist_node d_alias;  /* inode alias list */
};

typedef unsigned fmode_t;

struct file {
//    /*
//     * fu_list becomes invalid after file_free is called and queued via
//     * fu_rcuhead for RCU freeing
//     */
//    union {
//        struct list_head    fu_list;
//        struct rcu_head     fu_rcuhead;
//    } f_u;
//    struct path     f_path;
//#define f_dentry    f_path.dentry
    struct inode        *f_inode;   /* cached value */
//    const struct file_operations    *f_op;
//
//    /*
//     * Protects f_ep_links, f_flags.
//     * Must not be taken from IRQ context.
//     */
//    spinlock_t      f_lock;
//#ifdef __GENKSYMS__
//#ifdef CONFIG_SMP
//    int         f_sb_list_cpu;
//#endif
//#else
//#ifdef CONFIG_SMP
//    int         f_sb_list_cpu_deprecated;
//#endif
//#endif
//    atomic_long_t       f_count;
//    unsigned int        f_flags;
    fmode_t         f_mode;
    loff_t          f_pos;
//    struct fown_struct  f_owner;
//    const struct cred   *f_cred;
//    struct file_ra_state    f_ra;
//
//    u64         f_version;
//#ifdef CONFIG_SECURITY
//    void            *f_security;
//#endif
//    /* needed for tty driver, and maybe others */
//    void            *private_data;
//
//#ifdef CONFIG_EPOLL
//    /* Used by fs/eventpoll.c to link all the hooks to this file */
//    struct list_head    f_ep_links;
//    struct list_head    f_tfile_llink;
//#endif /* #ifdef CONFIG_EPOLL */
//    struct address_space    *f_mapping;
//#ifndef __GENKSYMS__
//    struct mutex        f_pos_lock;
//#endif
};

static inline struct inode *file_inode(const struct file *f)
{
    return f->f_inode;
}

#define dname(dentry)   ((char *)((dentry)->d_name.name))
#define dlen(dentry)    ((int)((dentry)->d_name.len))

extern void init_special_inode(struct inode *, umode_t, dev_t);
extern void truncate_inode_pages_range(struct address_space *, loff_t lstart, loff_t lend);


extern struct inode *igrab(struct inode *inode);
extern void iput(struct inode *inode);
extern void drop_nlink(struct inode *inode);
extern void clear_nlink(struct inode *inode);
extern void set_nlink(struct inode *inode, unsigned int nlink);
extern void inc_nlink(struct inode *inode);
extern struct inode *new_inode(struct super_block *sb);
extern void destroy_inode(struct inode* inode);
extern void inode_init_once(struct inode *inode);
extern int write_inode_now(struct inode *inode, int sync);
extern void remove_inode_hash(struct inode *inode);
extern boolean_t inode_owner_or_capable(const struct inode *inode);
extern void unlock_new_inode(struct inode *inode);
extern int insert_inode_locked(struct inode *inode);
extern void i_size_write(struct inode *inode, loff_t i_size);
extern void inode_set_flags(struct inode *inode, unsigned int flags, unsigned int mask);
extern void mark_inode_dirty(struct inode *inode);
extern void inode_set_iversion(struct inode *ip, uint64_t val);

extern struct dentry *d_make_root(struct inode *root_inode);
extern void d_prune_aliases(struct inode *inode);
extern void shrink_dcache_sb(struct super_block *sb);

extern boolean_t
zpl_dir_emit(zpl_dir_context_t *ctx, const char *name, int namelen, uint64_t ino, unsigned type);

extern void update_pages(znode_t *zp, int64_t start, int len, objset_t *os);
extern int mappedread(znode_t *zp, int nbytes, zfs_uio_t *uio);
extern uid_t zfs_uid_read(struct inode *inode);
extern gid_t zfs_gid_read(struct inode *inode);
extern void zfs_uid_write(struct inode *inode, uid_t uid);
extern void zfs_gid_write(struct inode *inode, gid_t gid);
extern void truncate_setsize(struct inode *inode, loff_t newsize);
extern int register_filesystem(struct file_system_type * fs);
extern int unregister_filesystem(struct file_system_type * fs);
extern void deactivate_super(struct super_block *s);


#define atomic_inc_not_zero(v)      atomic_add_unless((v), 1, 0)
extern int atomic_read(const atomic_t *v);
extern void atomic_set(atomic_t *v, int i);
extern int atomic_add_unless(atomic_t *v, int a, int u);

extern struct timespec timespec_trunc(struct timespec t, unsigned gran);
extern struct timespec current_kernel_time(void);
extern int timespec_compare(const struct timespec *lhs, const struct timespec *rhs);

#if !defined(HAVE_CURRENT_TIME)
extern struct timespec current_time(struct inode *inode);
#endif

extern struct timespec timespec_trunc(struct timespec t, unsigned gran);
extern struct timespec current_kernel_time(void);

extern int groupmember(gid_t gid, const cred_t *cr);
extern uid_t crgetfsuid(const cred_t *cr);
extern gid_t crgetfsgid(const cred_t *cr);

extern int fls(int x);
extern int ilog2(uint64_t n);

extern boolean_t has_capability(proc_t *t, int cap);
extern boolean_t capable(int cap);

extern long zfsdev_ioctl(unsigned cmd, unsigned long arg);
void schedule(void);

extern long zfsdev_ioctl_common(uint_t, zfs_cmd_t *, int);

// zfs_ctldir
extern struct inode *ilookup(struct super_block *sb, unsigned long ino);
extern struct dentry * d_obtain_alias(struct inode *);
extern boolean_t d_mountpoint(struct dentry *dentry);
extern void dput(struct dentry *);
extern int zfsctl_snapshot_unmount(const char *snapname, int flags);
extern int kern_path(const char *name, unsigned int flags, struct path *path);

extern void zfs_zero_partial_page(znode_t *zp, uint64_t start, uint64_t len);
extern void path_put(const struct path *path);

extern int generic_file_open(struct inode * inode, struct file * filp);

boolean_t zpl_dir_emit_dot(struct file *file, zpl_dir_context_t *ctx);
boolean_t zpl_dir_emit_dotdot(struct file *file, zpl_dir_context_t *ctx);
boolean_t zpl_dir_emit_dots(struct file *file, zpl_dir_context_t *ctx);
int zpl_root_readdir(struct file *filp, void *dirent, filldir_t filldir);
extern void generic_fillattr(struct inode *, struct linux_kstat *);

extern struct dentry * d_splice_alias(struct inode *, struct dentry *);

static inline void * ERR_PTR(long error)
{
        return (void *) error;
}

/* d_flags entries */
#define DCACHE_OP_HASH      0x0001
#define DCACHE_OP_COMPARE   0x0002
#define DCACHE_OP_REVALIDATE    0x0004
#define DCACHE_OP_DELETE    0x0008
#define DCACHE_OP_PRUNE         0x0010
/*
 * 2.6.38 API addition,
 * Added d_clear_d_op() helper function which clears some flags and the
 * registered dentry->d_op table.  This is required because d_set_d_op()
 * issues a warning when the dentry operations table is already set.
 * For the .zfs control directory to work properly we must be able to
 * override the default operations table and register custom .d_automount
 * and .d_revalidate callbacks.
 */
static inline void
d_clear_d_op(struct dentry *dentry)
{
    dentry->d_op = NULL;
    dentry->d_flags &= ~(
        DCACHE_OP_HASH | DCACHE_OP_COMPARE |
        DCACHE_OP_REVALIDATE | DCACHE_OP_DELETE);
}

extern void d_set_d_op(struct dentry *dentry, const struct dentry_operations *op);
extern void d_instantiate(struct dentry *, struct inode *);

extern void zpl_vap_init(vattr_t *vap, struct inode *dir, umode_t mode, cred_t *cr);

extern loff_t generic_file_llseek(struct file *file, loff_t offset, int whence);
extern ssize_t generic_read_dir(struct file *, char *, size_t, loff_t *);

/*
 * 4.11 API change
 * These macros are defined by kernel 4.11.  We define them so that the same
 * code builds under kernels < 4.11 and >= 4.11.  The macros are set to 0 so
 * that it will create obvious failures if they are accidentally used when built
 * against a kernel >= 4.11.
 */

#ifndef STATX_BASIC_STATS
#define STATX_BASIC_STATS   0
#endif

#ifndef AT_STATX_SYNC_AS_STAT
#define AT_STATX_SYNC_AS_STAT   0
#endif

/*
 * 4.11 API change
 * 4.11 takes struct path *, < 4.11 takes vfsmount *
 */

#ifdef HAVE_VFSMOUNT_IOPS_GETATTR
#define ZPL_GETATTR_WRAPPER(func)                   \
static int                              \
func(struct vfsmount *mnt, struct dentry *dentry, struct kstat *stat)   \
{                                   \
    struct path path = { .mnt = mnt, .dentry = dentry };        \
    return func##_impl(&path, stat, STATX_BASIC_STATS,      \
        AT_STATX_SYNC_AS_STAT);                 \
}
#elif defined(HAVE_PATH_IOPS_GETATTR)
#define ZPL_GETATTR_WRAPPER(func)                   \
static int                              \
func(const struct path *path, struct kstat *stat, u32 request_mask, \
    unsigned int query_flags)                       \
{                                   \
    return (func##_impl(path, stat, request_mask, query_flags));    \
}
#elif defined(HAVE_USERNS_IOPS_GETATTR)
#define ZPL_GETATTR_WRAPPER(func)                   \
static int                              \
func(struct user_namespace *user_ns, const struct path *path,   \
    struct kstat *stat, u32 request_mask, unsigned int query_flags) \
{                                   \
    return (func##_impl(user_ns, path, stat, request_mask, \
        query_flags));  \
}
#else
#error
#endif

extern int zpl_snapdir_readdir(struct file *filp, void *dirent, filldir_t filldir);

#endif	/* _SYS_KERNEL_H */
