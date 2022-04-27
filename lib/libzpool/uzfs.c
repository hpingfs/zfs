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
 * Copyright (c) 2016 Actifio, Inc. All rights reserved.
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
#include <sys/zfs_vnops.h>
#include <sys/zfs_znode.h>
#include <sys/dmu_objset.h>
#include <sys/zstd/zstd.h>
#include <sys/zvol.h>
#include <zfs_fletcher.h>
#include <zlib.h>

#include <sys/zpl.h>

int zvol_init(void) { return 0; }
void zvol_fini(void) {}

//void zfs_zevent_drain_all(int *count) {}
//zfs_file_t * zfs_zevent_fd_hold(int fd, minor_t *minorp, zfs_zevent_t **ze) { return NULL; }

//void zfs_ereport_taskq_fini(void) {}

//void fm_init(void) {}
//void fm_fini(void) {}

int zvol_set_volsize(const char *name, uint64_t volsize) { return 0; }
int zvol_set_snapdev(const char *ddname, zprop_source_t source, uint64_t snapdev) { return 0; }
int zvol_set_volmode(const char *ddname, zprop_source_t source, uint64_t volmode) { return 0; }
void zvol_create_cb(objset_t *os, void *arg, cred_t *cr, dmu_tx_t *tx) {}
int zvol_get_stats(objset_t *os, nvlist_t *nv) { return 0; }
zvol_state_handle_t *zvol_suspend(const char *name) { return NULL; }
int zvol_resume(zvol_state_handle_t *zv) { return 0; }
int zvol_check_volsize(uint64_t volsize, uint64_t blocksize) { return 0; }
int zvol_check_volblocksize(const char *name, uint64_t volblocksize) { return 0; }

int zpl_bdi_setup(struct super_block *sb, char *name) { return 0; };
void zpl_bdi_destroy(struct super_block *sb) {}

void zpl_prune_sb(int64_t nr_to_scan, void *arg) {};

int zfsvfs_parse_options(char *mntopts, vfs_t **vfsp) { return 0; }

void uzfs_vap_init(vattr_t *vap, struct inode *dir, umode_t mode, cred_t *cr)
{
	vap->va_mask = ATTR_MODE;
	vap->va_mode = mode;
	vap->va_uid = crgetfsuid(cr);

	if (dir && dir->i_mode & S_ISGID) {
		vap->va_gid = KGID_TO_SGID(dir->i_gid);
		if (S_ISDIR(mode))
			vap->va_mode |= S_ISGID;
	} else {
		vap->va_gid = crgetfsgid(cr);
	}
}

void zpl_generic_fillattr(struct user_namespace *user_ns, struct inode *inode, struct linux_kstat *stat)
{
//    stat->dev = inode->i_sb->s_dev;
    stat->ino = inode->i_ino;
    stat->mode = inode->i_mode;
    stat->nlink = inode->i_nlink;
    stat->uid = inode->i_uid;
    stat->gid = inode->i_gid;
//    stat->rdev = inode->i_rdev;
    stat->size = i_size_read(inode);
    stat->atime = inode->i_atime;
    stat->mtime = inode->i_mtime;
    stat->ctime = inode->i_ctime;
    stat->blksize = (1 << inode->i_blkbits);
    stat->blocks = inode->i_blocks;
}

static int cp_new_stat(struct kstat *stat, struct stat *statbuf)
{
    struct stat tmp;

//    if (!valid_dev(stat->dev) || !valid_dev(stat->rdev))
//        return -EOVERFLOW;
//#if BITS_PER_LONG == 32
//    if (stat->size > MAX_NON_LFS)
//        return -EOVERFLOW;
//#endif

//    INIT_STRUCT_STAT_PADDING(tmp);
//    tmp.st_dev = encode_dev(stat->dev);
    tmp.st_ino = stat->ino;
    if (sizeof(tmp.st_ino) < sizeof(stat->ino) && tmp.st_ino != stat->ino)
        return -EOVERFLOW;
    tmp.st_mode = stat->mode;
    tmp.st_nlink = stat->nlink;
    if (tmp.st_nlink != stat->nlink)
        return -EOVERFLOW;
    tmp.st_uid = stat->uid;
    tmp.st_gid = stat->gid;
//    SET_UID(tmp.st_uid, from_kuid_munged(current_user_ns(), stat->uid));
//    SET_GID(tmp.st_gid, from_kgid_munged(current_user_ns(), stat->gid));
//    tmp.st_rdev = encode_dev(stat->rdev);
    tmp.st_size = stat->size;
    tmp.st_atime = stat->atime.tv_sec;
    tmp.st_mtime = stat->mtime.tv_sec;
    tmp.st_ctime = stat->ctime.tv_sec;
//#ifdef STAT_HAVE_NSEC
//    tmp.st_atime_nsec = stat->atime.tv_nsec;
//    tmp.st_mtime_nsec = stat->mtime.tv_nsec;
//    tmp.st_ctime_nsec = stat->ctime.tv_nsec;
//#endif
    tmp.st_blocks = stat->blocks;
    tmp.st_blksize = stat->blksize;
    memcpy(statbuf,&tmp,sizeof(tmp));
    return 0;
}

int uzfs_stat(const char *fsname, const char *targetname, struct stat *statbuf)
{
    struct linux_kstat stat;
    memset(&stat, 0, sizeof(struct linux_kstat));

    zfsvfs_t *zfsvfs = NULL;
	vfs_t *vfs = NULL;
    objset_t *os = NULL;
	struct inode *root_inode = NULL;
    struct super_block* sb = NULL;
    struct dentry *dentry = NULL;
    struct path *path = NULL;
    int error = 0;

	vfs = kmem_zalloc(sizeof (vfs_t), KM_SLEEP);

    error = zfsvfs_create(fsname, B_FALSE, &zfsvfs);
    if (error) goto out;

	vfs->vfs_data = zfsvfs;
	zfsvfs->z_vfs = vfs;

    sb = kmem_zalloc(sizeof(struct super_block), KM_SLEEP);
    sb->s_fs_info = zfsvfs;

    zfsvfs->z_sb = sb;

    error = zfsvfs_setup(zfsvfs, B_TRUE);
    if (error) goto out;

    error = zfs_root(zfsvfs, &root_inode);
    if (error) goto out;

    dentry = kmem_zalloc(sizeof(struct dentry), KM_SLEEP);
    dentry->d_flags = 0;
    dentry->d_parent = NULL;
    dentry->d_name.name = targetname;
    dentry->d_name.len = strlen(targetname);
    dentry->d_sb = sb;
    pthread_spin_init(&dentry->d_lock, PTHREAD_PROCESS_PRIVATE);

    if (strcmp(dname(dentry), "/") == 0) {
        dentry->d_inode = root_inode;
    } else {
        ASSERT(root_inode->i_op->lookup(root_inode, dentry, 0) == dentry);
    }

    path = kmem_zalloc(sizeof(struct path), KM_SLEEP);
    path->dentry = dentry;

    error = zpl_getattr_impl(path, &stat, 0, 0);
    if (error) goto out;

    cp_new_stat(&stat, statbuf);

out:
    if (dentry) {
        if (dentry->d_inode != root_inode)
            iput(dentry->d_inode);
        pthread_spin_destroy(&dentry->d_lock);
        kmem_free(dentry, sizeof(struct dentry));
    }

    if (path) {
        kmem_free(path, sizeof(struct path));
    }

    if (root_inode) {
        iput(root_inode);
    }

	VERIFY(zfsvfs_teardown(zfsvfs, B_TRUE) == 0);
	os = zfsvfs->z_os;

	/*
	 * z_os will be NULL if there was an error in
	 * attempting to reopen zfsvfs.
	 */
	if (os != NULL) {
		/*
		 * Unset the objset user_ptr.
		 */
		mutex_enter(&os->os_user_ptr_lock);
		dmu_objset_set_user(os, NULL);
		mutex_exit(&os->os_user_ptr_lock);

		/*
		 * Finally release the objset
		 */
		dmu_objset_disown(os, B_TRUE, zfsvfs);
	}

    if (sb)
	    kmem_free(sb, sizeof (struct super_block));
    if(zfsvfs)
	    kmem_free(zfsvfs, sizeof (zfsvfs_t));
    if(vfs)
        kmem_free(vfs, sizeof (vfs_t));

    return error;
}

// FIXME(hping)
#define MAX_NUM_FS (100)
static zfsvfs_t *zfsvfs_array[MAX_NUM_FS];
static int zfsvfs_idx = 0;

int uzfs_init(const char* fsname, uint64_t *fsid)
{
    int error = 0;
    vfs_t *vfs = NULL;
    objset_t *os = NULL;
    struct super_block *sb = NULL;
    zfsvfs_t *zfsvfs = NULL;

	vfs = kmem_zalloc(sizeof (vfs_t), KM_SLEEP);

    error = zfsvfs_create(fsname, B_FALSE, &zfsvfs);
    if (error) goto out;

	vfs->vfs_data = zfsvfs;
	zfsvfs->z_vfs = vfs;

    sb = kmem_zalloc(sizeof(struct super_block), KM_SLEEP);
    sb->s_fs_info = zfsvfs;

    zfsvfs->z_sb = sb;

    error = zfsvfs_setup(zfsvfs, B_TRUE);
    if (error) goto out;

    *fsid = zfsvfs_idx;

    zfsvfs_array[zfsvfs_idx++] = zfsvfs;

    return 0;

out:
    if (sb)
	    kmem_free(sb, sizeof (struct super_block));
    if(vfs)
        kmem_free(vfs, sizeof (vfs_t));
    if(zfsvfs)
	    kmem_free(zfsvfs, sizeof (zfsvfs_t));
    return -1;
}

int uzfs_fini(uint64_t fsid)
{
    zfsvfs_t *zfsvfs = zfsvfs_array[fsid];
    objset_t *os = zfsvfs->z_os;
    vfs_t *vfs = zfsvfs->z_vfs;
    struct super_block *sb = zfsvfs->z_sb;

	VERIFY(zfsvfs_teardown(zfsvfs, B_TRUE) == 0);

	/*
	 * z_os will be NULL if there was an error in
	 * attempting to reopen zfsvfs.
	 */
	if (os != NULL) {
		/*
		 * Unset the objset user_ptr.
		 */
		mutex_enter(&os->os_user_ptr_lock);
		dmu_objset_set_user(os, NULL);
		mutex_exit(&os->os_user_ptr_lock);

		/*
		 * Finally release the objset
		 */
		dmu_objset_disown(os, B_TRUE, zfsvfs);
	}

    if (sb)
	    kmem_free(sb, sizeof (struct super_block));
    if(vfs)
        kmem_free(vfs, sizeof (vfs_t));
    if(zfsvfs)
	    kmem_free(zfsvfs, sizeof (zfsvfs_t));

    return 0;
}

int uzfs_getroot(uint64_t fsid, uint64_t* ino)
{
    int error = 0;
    struct inode* root_inode = NULL;
    zfsvfs_t *zfsvfs = zfsvfs_array[fsid];

    error = zfs_root(zfsvfs, &root_inode);
    if (error) goto out;

    *ino = root_inode->i_ino;

    iput(root_inode);

out:
    return error;
}

int uzfs_getattr(uint64_t fsid, uint64_t ino, struct stat* stat)
{
    int error = 0;
    znode_t *zp = NULL;
    zfsvfs_t *zfsvfs = zfsvfs_array[fsid];

    ZFS_ENTER(zfsvfs);

    error = zfs_zget(zfsvfs, ino, &zp);
    if (error) goto out;

    struct linux_kstat kstatbuf;
    memset(&kstatbuf, 0, sizeof(struct linux_kstat));

    error = zfs_getattr_fast(NULL, ZTOI(zp), &kstatbuf);
    if (error) goto out;

    cp_new_stat(&kstatbuf, stat);

out:
    if (zp)
        iput(ZTOI(zp));

    ZFS_EXIT(zfsvfs);
    return error;
}

int uzfs_setattr(uint64_t fsid, uint64_t ino, struct iattr* attr)
{
    return 0;
}

int uzfs_lookup(uint64_t fsid, uint64_t dino, const char* name, uint64_t* ino)
{
    int error = 0;
    znode_t *dzp = NULL;
    znode_t *zp = NULL;
    zfsvfs_t *zfsvfs = zfsvfs_array[fsid];

    ZFS_ENTER(zfsvfs);

    error = zfs_zget(zfsvfs, dino, &dzp);
    if (error) goto out;

    error = zfs_lookup(dzp, name, &zp, 0, NULL, NULL, NULL);
    if (error) goto out;

    *ino = ZTOI(zp)->i_ino;

out:
    if (zp)
        iput(ZTOI(zp));

    if (dzp)
        iput(ZTOI(dzp));

    ZFS_EXIT(zfsvfs);
    return error;
}

int uzfs_mkdir(uint64_t fsid, uint64_t dino, const char* name, umode_t mode, uint64_t *ino)
{
    int error = 0;
    znode_t *dzp = NULL;
    znode_t *zp = NULL;
    zfsvfs_t *zfsvfs = zfsvfs_array[fsid];

    ZFS_ENTER(zfsvfs);

    error = zfs_zget(zfsvfs, dino, &dzp);
    if (error) goto out;

    vattr_t vap;
    uzfs_vap_init(&vap, ZTOI(dzp), mode | S_IFDIR, NULL);

    error = zfs_mkdir(dzp, name, &vap, &zp, NULL, 0, NULL);
    if (error) goto out;

    *ino = ZTOI(zp)->i_ino;

out:
    if (zp)
        iput(ZTOI(zp));

    if (dzp)
        iput(ZTOI(dzp));

    ZFS_EXIT(zfsvfs);
    return error;
}

int uzfs_rmdir(uint64_t fsid, uint64_t dino, const char* name)
{
    int error = 0;
    znode_t *dzp = NULL;
    zfsvfs_t *zfsvfs = zfsvfs_array[fsid];

    ZFS_ENTER(zfsvfs);

    error = zfs_zget(zfsvfs, dino, &dzp);
    if (error) goto out;

    error = zfs_rmdir(dzp, name, NULL, NULL, 0);
    if (error) goto out;

out:

    if (dzp)
        iput(ZTOI(dzp));

    ZFS_EXIT(zfsvfs);
    return error;
}

int uzfs_readdir(uint64_t fsid, uint64_t ino, struct uzfs_dirent *dirp, uint64_t count)
{
    int error = 0;
    znode_t *zp = NULL;
    zfsvfs_t *zfsvfs = zfsvfs_array[fsid];

    ZFS_ENTER(zfsvfs);

    error = zfs_zget(zfsvfs, ino, &zp);
    if (error) goto out;

    zpl_dir_context_t ctx= ZPL_DIR_CONTEXT_INIT(NULL, uzfs_dir_emit, 0);

    error = zfs_readdir(ZTOI(zp), &ctx, NULL);
    if (error) goto out;

out:
    if (zp)
        iput(ZTOI(zp));

    ZFS_EXIT(zfsvfs);
    return error;
}

int uzfs_create(uint64_t fsid, uint64_t dino, const char* name, umode_t mode, uint64_t *ino)
{
    int error = 0;
    znode_t *dzp = NULL;
    znode_t *zp = NULL;
    zfsvfs_t *zfsvfs = zfsvfs_array[fsid];

    ZFS_ENTER(zfsvfs);

    error = zfs_zget(zfsvfs, dino, &dzp);
    if (error) goto out;

    vattr_t vap;
    uzfs_vap_init(&vap, ZTOI(dzp), mode | S_IFREG, NULL);

    error = zfs_create(dzp, name, &vap, 0, mode, &zp, NULL, 0, NULL);
    if (error) goto out;

    *ino = ZTOI(zp)->i_ino;

out:
    if (zp)
        iput(ZTOI(zp));

    if (dzp)
        iput(ZTOI(dzp));

    ZFS_EXIT(zfsvfs);
    return error;
}

int uzfs_remove(uint64_t fsid, uint64_t dino, const char* name)
{
    int error = 0;
    znode_t *dzp = NULL;
    zfsvfs_t *zfsvfs = zfsvfs_array[fsid];

    ZFS_ENTER(zfsvfs);

    error = zfs_zget(zfsvfs, dino, &dzp);
    if (error) goto out;

    error = zfs_remove(dzp, name, NULL, 0);
    if (error) goto out;

out:

    if (dzp)
        iput(ZTOI(dzp));

    ZFS_EXIT(zfsvfs);
    return error;
}

int uzfs_rename(uint64_t fsid, uint64_t sdino, const char* sname, uint64_t tdino, const char* tname)
{
    return 0;
}

int uzfs_read(uint64_t fsid, uint64_t ino, zfs_uio_t *uio, int ioflag)
{
    int error = 0;
    znode_t *zp = NULL;
    zfsvfs_t *zfsvfs = zfsvfs_array[fsid];

    ZFS_ENTER(zfsvfs);

    error = zfs_zget(zfsvfs, ino, &zp);
    if (error) goto out;

    error = zfs_read(zp, uio, ioflag, NULL);
    if (error) goto out;

out:
    if (zp)
        iput(ZTOI(zp));

    ZFS_EXIT(zfsvfs);
    return error;
}

int uzfs_write(uint64_t fsid, uint64_t ino, zfs_uio_t *uio, int ioflag)
{
    int error = 0;
    znode_t *zp = NULL;
    zfsvfs_t *zfsvfs = zfsvfs_array[fsid];

    ZFS_ENTER(zfsvfs);

    error = zfs_zget(zfsvfs, ino, &zp);
    if (error) goto out;

    error = zfs_write(zp, uio, ioflag, NULL);
    if (error) goto out;

out:
    if (zp)
        iput(ZTOI(zp));

    ZFS_EXIT(zfsvfs);
    return error;
}

int uzfs_fsync(uint64_t fsid, uint64_t ino, int syncflag)
{
    int error = 0;
    znode_t *zp = NULL;
    zfsvfs_t *zfsvfs = zfsvfs_array[fsid];

    ZFS_ENTER(zfsvfs);

    error = zfs_zget(zfsvfs, ino, &zp);
    if (error) goto out;

    error = zfs_fsync(zp, syncflag, NULL);
    if (error) goto out;

out:
    if (zp)
        iput(ZTOI(zp));

    ZFS_EXIT(zfsvfs);
    return error;
}


int uzfs_setxattr(uint64_t fsid, uint64_t ino, const char *name, const void *value, size_t size, int flags)
{
    int error = 0;
    znode_t *zp = NULL;
    zfsvfs_t *zfsvfs = zfsvfs_array[fsid];

    ZFS_ENTER(zfsvfs);

    error = zfs_zget(zfsvfs, ino, &zp);
    if (error) goto out;

    error = zpl_xattr_set(ZTOI(zp), name, value, size, 0);
    if (error) goto out;

out:
    if (zp)
        iput(ZTOI(zp));

    ZFS_EXIT(zfsvfs);
    return error;
}

int uzfs_getxattr(uint64_t fsid, uint64_t ino, const char *name, void *value, size_t size)
{
    int error = 0;
    znode_t *zp = NULL;
    zfsvfs_t *zfsvfs = zfsvfs_array[fsid];

    ZFS_ENTER(zfsvfs);

    error = zfs_zget(zfsvfs, ino, &zp);
    if (error) goto out;

    error = zpl_xattr_get(ZTOI(zp), name, value, size);
    if (error) goto out;

out:
    if (zp)
        iput(ZTOI(zp));

    ZFS_EXIT(zfsvfs);
    return error;
}


int uzfs_listxattr(uint64_t fsid, uint64_t ino, char *list, size_t size)
{
    int error = 0;
    return error;
}


