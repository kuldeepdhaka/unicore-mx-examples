/*
 * This file is part of the unicore-mx project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
 * Copyright (C) 2014 Kuldeep Singh Dhaka <kuldeepdhaka9@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <unicore-mx/stm32/rcc.h>
#include <unicore-mx/stm32/gpio.h>
#include <unicore-mx/stm32/crs.h>
#include <unicore-mx/usbd/usbd.h>
#include <unicore-mx/cm3/scb.h>
#include "cdcacm-target.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>

void cdcacm_target_init(void)
{
	/* use usb SOF as correction source for HSI48 */
	crs_autotrim_usb_enable();

	/* use HSI48 for USB */
	rcc_set_msi_range(RCC_CR_MSIRANGE_48MHZ);
	rcc_osc_on(RCC_MSI);

	/* usb HSI48 for system clock */
	rcc_set_sysclk_source(RCC_MSI);
}

const usbd_backend *cdcacm_target_usb_driver(void)
{
	return USBD_STM32_FSDEV;
}
