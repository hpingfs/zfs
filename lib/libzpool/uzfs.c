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
#include <sys/zfs_znode.h>
#include <sys/zstd/zstd.h>
#include <sys/zvol.h>
#include <zfs_fletcher.h>
#include <zlib.h>

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

