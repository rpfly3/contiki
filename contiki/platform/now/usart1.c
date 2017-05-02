#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_usart.h>
#include <misc.h>
#include <core_cm3.h>
#include <stdio.h>

void USART1_GPIO_Config()
{
	GPIO_InitTypeDef gpioConfig;

	//PA9 = USART1.TX => Alternative Function Output
	gpioConfig.GPIO_Mode = GPIO_Mode_AF_PP;
	gpioConfig.GPIO_Pin = GPIO_Pin_9;
	gpioConfig.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpioConfig);

	//PA10 = USART1.RX => Input
	gpioConfig.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	gpioConfig.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIOA, &gpioConfig);
}

void USART1_NVIC_Config()
{
	NVIC_InitTypeDef NVIC_InitStructure;
	/* Set priority group 1 */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void InitializeUSART1()
{
	USART_InitTypeDef usartConfig;
	USART_DeInit(USART1);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	
	USART1_GPIO_Config();
	
	usartConfig.USART_BaudRate = 115200; 
	usartConfig.USART_WordLength = USART_WordLength_8b; 
	usartConfig.USART_StopBits = USART_StopBits_1; 
	usartConfig.USART_Parity = USART_Parity_No;
	usartConfig.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	usartConfig.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(USART1, &usartConfig);
	
	USART1_NVIC_Config();
	
    /* Enable USART1 Receive interrupt */
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART1, ENABLE); 
}

void USART1_IRQHandler(void)
{	
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
		uint8_t rx = USART_ReceiveData(USART1);
		USART_SendData(USART1, rx);
		while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
			;

		if (rx == 0x4C) {
			NVIC_SystemReset();
		}
	}
}