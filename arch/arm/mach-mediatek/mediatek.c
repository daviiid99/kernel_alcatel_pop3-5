/*
 * Device Tree support for Mediatek SoCs
 *
 * Copyright (c) 2014 MundoReader S.L.
 * Author: Matthias Brugger <matthias.bgg@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
<<<<<<< HEAD
 * the Free Software Foundation; either version 2 of the License.
=======
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/init.h>
#include <asm/mach/arch.h>

static const char * const mediatek_board_dt_compat[] = {
	"mediatek,mt6589",
	NULL,
};

DT_MACHINE_START(MEDIATEK_DT, "Mediatek Cortex-A7 (Device Tree)")
	.dt_compat	= mediatek_board_dt_compat,
MACHINE_END
