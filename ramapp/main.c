/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018, Erik Moqvist
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * This file is part of the PIC tools project.
 */

#include "simba.h"
#include "ramapp.h"

static void clock_init(void)
{
    /* Unlock. */
    pic32mm_reg_write(&PIC32MM_RDS_CONF->SYSKEY, PIC32MM_RDS_CONF_SYSKEY_LOCK);
    pic32mm_reg_write(&PIC32MM_RDS_CONF->SYSKEY, PIC32MM_RDS_CONF_SYSKEY_UNLOCK_1);
    pic32mm_reg_write(&PIC32MM_RDS_CONF->SYSKEY, PIC32MM_RDS_CONF_SYSKEY_UNLOCK_2);

    /* Use FRC as input to PLL and multiple by 3 to 24 MHz SPLL. */
    pic32mm_reg_write(&PIC32MM_OSC->SPLLCON, (PIC32MM_OSC_SPLLCON_PLLICLK
                                              | PIC32MM_OSC_SPLLCON_PLLMULT_3));

    /* Select SPLL as clock. */
    pic32mm_reg_write(&PIC32MM_OSC->OSCCON, PIC32MM_OSC_OSCCON_NOSC_SPLL);

    /* Perform the clock switch. */
    pic32mm_reg_set(&PIC32MM_OSC->OSCCON, PIC32MM_OSC_OSCCON_OSWEN);

    while (pic32mm_reg_read(&PIC32MM_OSC->OSCCON) & PIC32MM_OSC_OSCCON_OSWEN);

    /* Lock. */
    pic32mm_reg_write(&PIC32MM_RDS_CONF->SYSKEY, PIC32MM_RDS_CONF_SYSKEY_LOCK);
}

int main()
{
    struct ramapp_t ramapp;
    struct flash_driver_t flash;

    clock_init();

    flash_module_init();
    flash_init(&flash, &flash_device[0]);
    ramapp_init(&ramapp, &flash);

    while (1) {
        ramapp_process_packet(&ramapp);
    }

    return (0);
}
