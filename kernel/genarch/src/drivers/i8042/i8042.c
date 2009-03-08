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

/** @addtogroup genarch	
 * @{
 */
/**
 * @file
 * @brief	i8042 processor driver
 *
 * It takes care of the i8042 serial communication.
 */

#include <genarch/drivers/i8042/i8042.h>
#include <genarch/drivers/legacy/ia32/io.h>
#include <arch/asm.h>
#include <console/chardev.h>
#include <mm/slab.h>

#define i8042_SET_COMMAND 	0x60
#define i8042_COMMAND 		0x69

#define i8042_BUFFER_FULL_MASK	0x01
#define i8042_WAIT_MASK		0x02

static irq_ownership_t i8042_claim(irq_t *irq)
{
	i8042_instance_t *i8042_instance = irq->instance;
	i8042_t *dev = i8042_instance->i8042;
	if (pio_read_8(&dev->status) & i8042_BUFFER_FULL_MASK)
		return IRQ_ACCEPT;
	else
		return IRQ_DECLINE;
}

static void i8042_irq_handler(irq_t *irq)
{
	i8042_instance_t *instance = irq->instance;
	i8042_t *dev = instance->i8042;

	uint8_t data;
	uint8_t status;
		
	if (((status = pio_read_8(&dev->status)) & i8042_BUFFER_FULL_MASK)) {
		data = pio_read_8(&dev->data);
			
		if (instance->devout)
			chardev_push_character(instance->devout, data);
	}
}

/** Initialize i8042. */
bool
i8042_init(i8042_t *dev, devno_t devno, inr_t inr, chardev_t *devout)
{
	i8042_instance_t *instance;

	instance = malloc(sizeof(i8042_instance_t), FRAME_ATOMIC);
	if (!instance)
		return false;
	
	instance->devno = devno;
	instance->i8042 = dev;
	instance->devout = devout;
	
	irq_initialize(&instance->irq);
	instance->irq.devno = devno;
	instance->irq.inr = inr;
	instance->irq.claim = i8042_claim;
	instance->irq.handler = i8042_irq_handler;
	instance->irq.instance = instance;
	irq_register(&instance->irq);
	
	/*
	 * Clear input buffer.
	 */
	while (pio_read_8(&dev->status) & i8042_BUFFER_FULL_MASK)
		(void) pio_read_8(&dev->data);
	
	return true;
}

/** @}
 */