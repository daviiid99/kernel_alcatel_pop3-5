/*
 *  Copyright (C) 2010, Lars-Peter Clausen <lars@metafoo.de>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General	 Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __MIPS_JZ4740_IRQ_H__
#define __MIPS_JZ4740_IRQ_H__

#include <linux/irq.h>
<<<<<<< HEAD

=======
#include <asm/mach-jz4740/irq.h>

struct irq_data;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
extern void jz4740_irq_suspend(struct irq_data *data);
extern void jz4740_irq_resume(struct irq_data *data);

#endif
