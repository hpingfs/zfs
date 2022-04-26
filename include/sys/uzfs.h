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
#include <sys/vnode.h>

#define zpl_fs_type                            uzfs_fs_type
#define zpl_super_operations                   uzfs_super_operations
#define zpl_export_operations                  uzfs_export_operations
//#define zpl_dentry_operations                  uzfs_dentry_operations

//#define zpl_file_operations                    uzfs_file_operations
//#define zpl_dir_file_operations                uzfs_dir_file_operations
//#define zpl_address_space_operations           uzfs_address_space_operations

//#define zpl_inode_operations                   uzfs_inode_operations
//#define zpl_dir_inode_operations               uzfs_dir_inode_operations
//#define zpl_symlink_inode_operations           uzfs_symlink_inode_operations
//#define zpl_special_inode_operations           uzfs_special_inode_operations

// zfs_ctldir
//#define zpl_fops_root                          uzfs_fops_root
//#define zpl_ops_root                           uzfs_ops_root
//#define zpl_fops_snapdir                       uzfs_fops_snapdir
//#define zpl_ops_snapdir                        uzfs_ops_snapdir
//#define zpl_fops_shares                        uzfs_fops_shares
//#define zpl_ops_shares                         uzfs_ops_shares

extern void zpl_prune_sb(int64_t nr_to_scan, void *arg);

extern void uzfs_vap_init(vattr_t *vap, struct inode *dir, umode_t mode, cred_t *cr);

#define zfs_uio_fault_disable(u, set)

#if defined(HAVE_INODE_OWNER_OR_CAPABLE)
#define zpl_inode_owner_or_capable(ns, ip)  inode_owner_or_capable(ip)
#elif defined(HAVE_INODE_OWNER_OR_CAPABLE_IDMAPPED)
#define zpl_inode_owner_or_capable(ns, ip)  inode_owner_or_capable(ns, ip)
#else
#error "Unsupported kernel"
#endif

#define zvol_tag(zv) (NULL)

typedef struct vfs vfs_t;
extern int zfsvfs_parse_options(char *mntopts, vfs_t **vfsp);

struct super_block;
int zpl_bdi_setup(struct super_block *sb, char *name);
void zpl_bdi_destroy(struct super_block *sb);

extern void zpl_generic_fillattr(struct user_namespace *user_ns, struct inode *inode, struct linux_kstat *stat);

int uzfs_stat(const char *fsname, const char* path, struct stat *buf);

int uzfs_init(const char* fsname);
int uzfs_fini(const char* fsname);

int uzfs_getroot(const char *fsname, uint64_t* ino);
int uzfs_getattr(uint64_t ino, struct stat* stat);
int uzfs_setattr(uint64_t ino, struct iattr* attr);
int uzfs_lookup(uint64_t dino, const char* name, uint64_t* ino);
int uzfs_mkdir(uint64_t dino, const char* name, umode_t mode, uint64_t *ino);
int uzfs_rmdir(uint64_t dino, const char* name);
//int uzfs_readdir(uint64_t dino, struct linux_dirent *dirp, uint64_t count);
int uzfs_create(uint64_t dino, const char* name, umode_t mode, uint64_t *ino);
int uzfs_remove(uint64_t dino, const char* name);
int uzfs_rename(uint64_t sdino, const char* sname, uint64_t tdino, const char* tname);

int uzfs_read(uint64_t ino, zfs_uio_t *uio, int ioflag);
int uzfs_write(uint64_t ino, zfs_uio_t *uio, int ioflag);
int uzfs_fsync(uint64_t ino, int syncflag);

#endif	/* _SYS_UZFS_H */
