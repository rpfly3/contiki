#include "sys/rtimer.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "misc.h"
#include <stdio.h>
#include "contiki.h"

PROCESS(rtimer_process, "Rtimer process");
static uint32_t network_time_msb = 0; // Most significant bits of the current time.
static rtimer_clock_t next_rtimer_time = 0;
static uint32_t target_time_msb = 0;
static uint16_t target_time_lsb = 0;

/**
 * @brief   This function sets the current network time to network_time (mainly for time sync purpose).
 * @param   rtimer_clock_t network_time
 * @retval  None
 */
void network_time_set(rtimer_clock_t network_time) {
    network_time_msb = network_time >> 16;
    TIM_SetCounter(TIM3, (uint16_t)network_time);
}

/**
 * @brief   This function handles TIM2 global interrupt request.
 * @param   None
 * @retval  None
 */
void TIM3_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
        ++network_time_msb;
    } 
	if(TIM_GetITStatus(TIM3, TIM_IT_CC1) != RESET) {
        TIM_ClearITPendingBit(TIM3, TIM_IT_CC1);
		if (network_time_msb == target_time_msb)
		{
			TIM_ITConfig(TIM3, TIM_IT_CC1, DISABLE);
			process_poll(&rtimer_process);
		}
    } 
}

/**
 * @brief   Configures the different system clocks.
 * @param   None
 * @retval  None
 */
void RCC_Configuration(void) {
    /* TIM3 clock enable */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    /* GPIOC clock enable */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
}

/**
 * @brief   Configure the TIM3 Pins.
 * @param   None
 * @retval  None
 */
void GPIO_Configuration(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    /* GPIOC Configuration: TIM3 channel 1, 2, 3, and 4 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_PinRemapConfig(GPIO_FullRemap_TIM3, ENABLE);
}

/**
 * @brief   Configure the nested vectored interrupt controller.
 * @param   None
 * @retval  None
 */
void NVIC_Configuration(void) {
    NVIC_InitTypeDef NVIC_InitStructure;
    /* Enable the TIM2 global interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void rtimer_arch_init(void) {

    /* System Clocks Configuration */
    RCC_Configuration();

    /* NVIC Configuration */
    NVIC_Configuration();

    /* GPIO Configuration */
    GPIO_Configuration();

    /* ---------------------------------------------------
     TIM3 Configuration: Output Compare Toggle Mode:
     TIM3CLK = SystemCoreClock, (because APB1 no prescaler)
     The objective is to get TIM3 counter clock at 1 MHz:
      - Prescaler = (TIM3CLK / TIM3 counter clock) - 1
     ----------------------------------------------------*/
    /* Cmpute the prescaler value */
    uint16_t PrescalerValue = (uint16_t)(SystemCoreClock / 1000000) - 1;

    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	TIM_TimeBaseStructure.TIM_Period = 65535;
    TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    TIM_OCInitTypeDef TIM_OCInitStructure;
    /* Output Compare Toggle Mode configuration: Channel 1 */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

    TIM_OC1Init(TIM3, &TIM_OCInitStructure);

    /* disable preload register, such that shadow register is updated immediately */
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Disable);
	
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	/* TIM2 enable counter */
	TIM_Cmd(TIM3, ENABLE);
	
    /* TIM IT enable */
	// Note that TIM_IT_CC1 is not enabled yet
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

}

rtimer_clock_t rtimer_arch_now(void) {
    uint16_t capture = TIM_GetCounter(TIM3);
    return (rtimer_clock_t)((network_time_msb << 16) | capture);
}

void rtimer_arch_schedule(rtimer_clock_t t) {
    next_rtimer_time = t;
	target_time_msb = t >> 16;
	target_time_lsb = t & 0xFFFF;

	TIM_SetCompare1(TIM3, target_time_lsb);
	TIM_ITConfig(TIM3, TIM_IT_CC1, ENABLE);
}

PROCESS_THREAD(rtimer_process, ev, data)
{
	PROCESS_BEGIN();
	while (1)
	{
		PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
        rtimer_run_next();
	}
	PROCESS_END();
}