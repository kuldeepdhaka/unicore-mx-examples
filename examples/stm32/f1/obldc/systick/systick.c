/*
 * This file is part of the unicore-mx project.
 *
 * Copyright (C) 2010 Thomas Otto <tommi@viadmin.org>
 * Copyright (C) 2010 Piotr Esden-Tempski <piotr@esden.net>
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

#include <unicore-mx/stm32/rcc.h>
#include <unicore-mx/stm32/flash.h>
#include <unicore-mx/stm32/gpio.h>
#include <unicore-mx/cm3/nvic.h>
#include <unicore-mx/cm3/systick.h>

uint32_t temp32;

static void gpio_setup(void)
{
	/* Enable GPIOA clock. */
	rcc_periph_clock_enable(RCC_GPIOA);

	/* Enable GPIOB clock. */
	rcc_periph_clock_enable(RCC_GPIOB);

	gpio_set(GPIOA, GPIO6); /* LED0 off */
	gpio_set(GPIOA, GPIO7); /* LED1 off */
	gpio_set(GPIOB, GPIO0); /* LED2 off */
	gpio_set(GPIOB, GPIO1); /* LED3 off */

	/* Set GPIO6/7 (in GPIO port A) to 'output push-pull' for the LEDs. */
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO6);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO7);

	/* Set GPIO0/1 (in GPIO port B) to 'output push-pull' for the LEDs. */
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO0);
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO1);
}

void sys_tick_handler(void)
{
	temp32++;

	/* We call this handler every 1ms so 1000ms = 1s on/off. */
	if (temp32 == 1000) {
		gpio_toggle(GPIOA, GPIO6); /* LED2 on/off */
		temp32 = 0;
	}
}

int main(void)
{
	rcc_clock_setup_in_hse_8mhz_out_72mhz();
	gpio_setup();

	gpio_clear(GPIOA, GPIO7);	/* LED1 on */
	gpio_set(GPIOA, GPIO6);		/* LED2 off */

	temp32 = 0;

	/* 72MHz / 8 => 9000000 counts per second */
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);

	/* 9000000/9000 = 1000 overflows per second - every 1ms one interrupt */
	/* SysTick interrupt every N clock pulses: set reload to N-1 */
	systick_set_reload(8999);

	systick_interrupt_enable();

	/* Start counting. */
	systick_counter_enable();

	while (1); /* Halt. */

	return 0;
}
