/*
 * Copyright (c) 2009 Jakub Jermar 
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

/** @addtogroup arm32
 * @{
 */
/** @file
 *  @brief Kernel part of Restartable Atomic Sequences support.
 */

#include <arch/ras.h>
#include <mm/mm.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <mm/tlb.h>
#include <mm/asid.h>
#include <interrupt.h>
#include <arch/exception.h>
#include <arch.h>
#include <memstr.h>
#include <arch/types.h>

uintptr_t *ras_page = NULL;

void ras_init(void)
{
	ras_page = frame_alloc(ONE_FRAME, FRAME_KA);
	memsetb(ras_page, FRAME_SIZE, 0); 
	ras_page[RAS_START] = 0;
	ras_page[RAS_END] = 0xffffffff;
	/*
	 * Userspace needs to be able to write to this page. The page is 
	 * cached in TLB as PAGE_KERNEL. Purge it from TLB and map it
	 * read/write PAGE_USER.
	 */
	tlb_invalidate_pages(ASID_KERNEL, (uintptr_t)ras_page, 1);
	page_table_lock(AS, true);
	page_mapping_insert(AS, (uintptr_t)ras_page, (uintptr_t)KA2PA(ras_page),
	    PAGE_READ | PAGE_WRITE | PAGE_USER);
	page_table_unlock(AS, true);
}

void ras_check(int n, istate_t *istate)
{
	uintptr_t rewrite_pc = istate->pc;

	if (istate_from_uspace(istate)) {
		if (ras_page[RAS_START]) {
			if ((ras_page[RAS_START] < istate->pc) &&
			    (ras_page[RAS_END] > istate->pc)) {
				rewrite_pc = ras_page[RAS_START];
			}
			ras_page[RAS_START] = 0;
			ras_page[RAS_END] = 0xffffffff;
		}	
	}

	exc_dispatch(n, istate);

	istate->pc = rewrite_pc;
}
