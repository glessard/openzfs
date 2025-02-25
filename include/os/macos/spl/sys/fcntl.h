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
 *
 * Copyright (C) 2013 Jorgen Lundman <lundman@lundman.net>
 *
 */

#ifndef _SPL_FCNTL_H
#define	_SPL_FCNTL_H

#include <AvailabilityMacros.h>
#if !defined(MAC_OS_X_VERSION_10_9) ||	\
	(MAC_OS_X_VERSION_MIN_REQUIRED <= MAC_OS_X_VERSION_10_9)
#include <i386/types.h>
#endif

#include_next <sys/fcntl.h>

#define	F_FREESP		11

#define	O_LARGEFILE		0
#define	O_RSYNC			0
#define	O_DIRECT		0

#endif /* _SPL_FCNTL_H */
