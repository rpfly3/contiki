/**
 * \addtogroup mbxxx-platform
 *
 * @{
 */

/*
 * Copyright (c) 2010, STMicroelectronics.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
* \file
*			Watchdog
* \author
*			Salvatore Pitrulli <salvopitru@users.sourceforge.net>
*/

#include <stm32f10x_iwdg.h>
#include "core/dev/watchdog.h"

#define LSI_FREQ 40000
/*---------------------------------------------------------------------------*/
void watchdog_init(void)
{
	/* IWDG timeout equal to 1600 ms (the timeout may varies due to LSI frequency dispersion) */

	/* Enable write access to IWDG_PR and IWDG_RLR registers */
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

	/* IWDG counter clock: LSI/32 */
	IWDG_SetPrescaler(IWDG_Prescaler_32);

	 /* Set counter reload value to obtain 250ms IWDG TimeOut.
     Counter Reload Value = 1600ms / IWDG counter clock period
                          = 1.6s / (LSI / 32)
                          = 1.6s / (LSI_FREQ / 32)
                          = LsiFreq/(32 / 2)
                          = LsiFreq/20
   */
	IWDG_SetReload(LSI_FREQ / 20);
}
/*---------------------------------------------------------------------------*/
void watchdog_start(void)
{
  /* 
   * We setup the watchdog to reset the device after 2.048 seconds,
   * unless watchdog_periodic() is called.
   */
	IWDG_ReloadCounter();
	IWDG_Enable();
}
/*---------------------------------------------------------------------------*/
void watchdog_periodic(void)
{
	/* This function is called periodically to restart the watchdog timer. */
	IWDG_ReloadCounter();
}
/*---------------------------------------------------------------------------*/
void watchdog_stop(void)
{
}
/*---------------------------------------------------------------------------*/
void watchdog_reboot(void)
{
}
/*---------------------------------------------------------------------------*/
/** @} */
