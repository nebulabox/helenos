/*
 * Copyright (c) 2006 Josef Cejka
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

/** @addtogroup kbd_port
 * @ingroup  kbd
 * @{
 */ 
/** @file
 * @brief	NS16550 port driver.
 */

#include <ipc/ipc.h>
#include <async.h>
#include <sysinfo.h>
#include <kbd.h>
#include <kbd_port.h>
#include <ddi.h>

/* NS16550 registers */
#define RBR_REG		0	/** Receiver Buffer Register. */
#define IER_REG		1	/** Interrupt Enable Register. */
#define IIR_REG		2	/** Interrupt Ident Register (read). */
#define FCR_REG		2	/** FIFO control register (write). */
#define LCR_REG		3	/** Line Control register. */
#define MCR_REG		4	/** Modem Control Register. */
#define LSR_REG		5	/** Line Status Register. */

irq_cmd_t ns16550_cmds[1] = {
	{ CMD_PORT_READ_1, 0, 0, 2 },
};

irq_code_t ns16550_kbd = {
	1,
	ns16550_cmds
};

static void ns16550_irq_handler(ipc_callid_t iid, ipc_call_t *call);

uint16_t ns16550_port;

int kbd_port_init(void)
{
	async_set_interrupt_received(ns16550_irq_handler);

	ns16550_port = sysinfo_value("kbd.port");
	ns16550_kbd.cmds[0].addr = (void *) (ns16550_port + RBR_REG);
	ipc_register_irq(sysinfo_value("kbd.inr"), sysinfo_value("kbd.devno"),
	    0, &ns16550_kbd);
	iospace_enable(task_get_id(), ns16550_port, 8);

	return 0;
}

#define LSR_DATA_READY	0x01

static void ns16550_irq_handler(ipc_callid_t iid, ipc_call_t *call)
{
	int scan_code = IPC_GET_ARG2(*call);
	kbd_push_scancode(scan_code);
}

/**
 * @}
 */