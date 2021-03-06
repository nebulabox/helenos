/*
 * Copyright (c) 2010 Jakub Jermar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup generic
 * @{
 */
/** @file
 */

#ifndef KERN_PANIC_H_
#define KERN_PANIC_H_

#include <stdbool.h>
#include <stddef.h>
#include <typedefs.h>
#include <print.h>

#define panic(fmt, ...) \
	panic_common(PANIC_OTHER, NULL, 0, 0, fmt, ##__VA_ARGS__)

#define panic_assert(fmt, ...) \
	panic_common(PANIC_ASSERT, NULL, 0, 0, fmt, ##__VA_ARGS__)

#define panic_badtrap(istate, n, fmt, ...) \
	panic_common(PANIC_BADTRAP, istate, 0, n, fmt, ##__VA_ARGS__)

#define panic_memtrap(istate, access, addr, fmt, ...) \
	panic_common(PANIC_MEMTRAP, istate, access, addr, fmt, ##__VA_ARGS__)

typedef enum {
	PANIC_OTHER,
	PANIC_ASSERT,
	PANIC_BADTRAP,
	PANIC_MEMTRAP
} panic_category_t;

struct istate;

extern bool console_override;

extern void panic_common(panic_category_t, struct istate *, int,
    uintptr_t, const char *, ...) __attribute__ ((noreturn))
    PRINTF_ATTRIBUTE(5, 6);

#endif

/** @}
 */
