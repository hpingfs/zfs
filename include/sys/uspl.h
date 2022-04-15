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

#ifndef	_SYS_USPL_H
#define	_SYS_USPL_H

// vmsystm.h
// FIXME(hping) should query kernel
extern unsigned long totalram_pages;
#define zfs_totalram_pages  totalram_pages

// sysmacros.h
#define is_system_labeled()     0

// cred.h
#define	KUID_TO_SUID(x)		(__kuid_val(x))
#define	KGID_TO_SGID(x)		(__kgid_val(x))
#define	SUID_TO_KUID(x)		(KUIDT_INIT(x))
#define	SGID_TO_KGID(x)		(KGIDT_INIT(x))
#define	KGIDP_TO_SGIDP(x)	(&(x)->val)

// FIXME(hping)
//#define current (0)

#define	mutex_owner(mp)		(((mp)->m_owner))
#define	mutex_owned(mp)		pthread_equal((mp)->m_owner, pthread_self())
#define	MUTEX_HELD(mp)		mutex_owned(mp)
#define	MUTEX_NOT_HELD(mp)	(!MUTEX_HELD(mp))

struct task_struct {};

int ddi_copyin(const void *from, void *to, size_t len, int flags);
int ddi_copyout(const void *from, void *to, size_t len, int flags);
int xcopyin(const void *from, void *to, size_t len);
int xcopyout(const void *from, void *to, size_t len);
int copyinstr(const void *from, void *to, size_t len, size_t *done);

boolean_t zfs_proc_is_caller(struct task_struct *t);

#endif	/* _SYS_USPL_H */
