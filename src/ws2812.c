/*
 * ws2812.c
 *
 *  Created on: 19.06.2014
 *      Author: pyro
 */
#include "ws2812.h"
#include "stm32f10x_conf.h"
#include "colors.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

static DMA_InitTypeDef dmaConfig;
static volatile bool transferRunning = false;

#define LEDBUFFSIZE (3 * 8 * LED + 80)
static uint16_t ledBuffer[LEDBUFFSIZE];


// Timing Definitions
#define PERIODE		(31)
#define LOGIC_ONE ((uint16_t) (20))
#define LOGIC_ZERO ((uint16_t) (10))


void WS2812_Init() {
	GPIO_InitTypeDef gpioConfig;
	TIM_TimeBaseInitTypeDef timConfig;
	TIM_OCInitTypeDef TIM_OCInitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_StructInit(&gpioConfig);
    gpioConfig.GPIO_Pin = GPIO_Pin_6;
	gpioConfig.GPIO_Mode = GPIO_Mode_AF_PP;
	gpioConfig.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_Init(GPIOA, &gpioConfig);

	// Enable Timer
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	/* Compute the prescaler value for 24 MHz -> 30 * 24 MHz = 1,25 us */
	uint16_t prescaler = (uint16_t) (SystemCoreClock / 24000000) - 1;
	/* Time base configuration */
	timConfig.TIM_Period = (PERIODE - 1); // 800kHz
	timConfig.TIM_Prescaler = prescaler;
	timConfig.TIM_ClockDivision = 0;
	timConfig.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &timConfig);

	/* PWM1 Mode configuration: Channel1 */
	TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    //TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    //TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OC1Init(TIM3, &TIM_OCInitStructure);

	// Configure dma
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	/* DMA1 Channel6 Config */
	DMA_DeInit(DMA1_Channel6);

	DMA_StructInit(&dmaConfig);
	dmaConfig.DMA_PeripheralBaseAddr = (uint32_t) &(TIM3->CCR1);
	dmaConfig.DMA_DIR = DMA_DIR_PeripheralDST;
	dmaConfig.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	dmaConfig.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dmaConfig.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
    dmaConfig.DMA_MemoryDataSize = DMA_PeripheralDataSize_Word;
	dmaConfig.DMA_Mode = DMA_Mode_Normal;
	dmaConfig.DMA_Priority = DMA_Priority_Medium;
	dmaConfig.DMA_M2M = DMA_M2M_Disable;

	// DMA config
	dmaConfig.DMA_MemoryBaseAddr = (uint32_t) ledBuffer;
	dmaConfig.DMA_BufferSize = LEDBUFFSIZE;
    DMA_Init(DMA1_Channel6, &dmaConfig);

	/* TIM3 CC1 DMA Request enable */
	TIM_DMACmd(TIM3, TIM_DMA_CC1, ENABLE);

	// Enable DMA Finished Interrupt
	DMA_ClearITPendingBit(DMA1_IT_TC6);
	DMA_ITConfig(DMA1_Channel6, DMA_IT_TC, ENABLE);

	NVIC_ClearPendingIRQ(DMA1_Channel6_IRQn);
	NVIC_EnableIRQ(DMA1_Channel6_IRQn);

    transferRunning = false;
}

/* This function sends data bytes out to a string of WS2812s
 * The first argument is a pointer to the first RGB triplet to be sent
 * The seconds argument is the number of LEDs in the chain
 *
 * This will result in the RGB triplet passed by argument 1 being sent to
 * the LED that is the furthest away from the controller (the point where
 * data is injected into the chain)
 */
void WS2812_send(uint16_t (*color)[3], uint16_t len) {
	// wait for running transfers
	while (transferRunning)
		;

	uint32_t memaddr = 0;	// reset buffer memory index
	uint32_t led = 0;	// reset led index

    // fill transmit buffer with correct compare values to achieve
    // correct pulse widths according to color values
	while (len) {
        for (uint8_t j = 0; j < 8; j++)	// GREEN data
		{
			if ((color[led][1] << j) & 0x80)// data sent MSB first, j = 0 is MSB j = 7 is LSB
			{
				ledBuffer[memaddr] = LOGIC_ONE; // compare value for logical 1
			} else {
				ledBuffer[memaddr] = LOGIC_ZERO;	// compare value for logical 0
			}
			memaddr++;
		}

        for (uint8_t j = 0; j < 8; j++)	// RED data
		{
			if ((color[led][0] << j) & 0x80)// data sent MSB first, j = 0 is MSB j = 7 is LSB
			{
				ledBuffer[memaddr] = LOGIC_ONE; // compare value for logical 1
			} else {
				ledBuffer[memaddr] = LOGIC_ZERO;	// compare value for logical 0
			}
			memaddr++;
		}

        for (uint8_t j = 0; j < 8; j++)	// BLUE data
		{
			if ((color[led][2] << j) & 0x80)// data sent MSB first, j = 0 is MSB j = 7 is LSB
			{
				ledBuffer[memaddr] = LOGIC_ONE; // compare value for logical 1
			} else {
				ledBuffer[memaddr] = LOGIC_ZERO;	// compare value for logical 0
			}
			memaddr++;
		}

		led++;
		len--;
	}

    // deactivate remaining leds
//    for(; led < LED; led++){
//        ledBuffer[memaddr++] = LOGIC_ZERO;
//        ledBuffer[memaddr++] = LOGIC_ZERO;
//        ledBuffer[memaddr++] = LOGIC_ZERO;
//    }

    // add needed delay at end of byte cycle, pulsewidth = 0
    for(; memaddr < LEDBUFFSIZE; memaddr++){
		ledBuffer[memaddr] = 0;
	}

	//DMA_SetCurrDataCounter(DMA1_Channel6, buffersize); // load number of bytes to be transferred
    transferRunning = true;

    // DMA (needed to be reintialized -> NO DEINIT!!)
    DMA_Init(DMA1_Channel6, &dmaConfig);
	DMA_Cmd(DMA1_Channel6, ENABLE); // enable DMA channel 6

	TIM_Cmd(TIM3, ENABLE); // enable Timer 3
}

void DMA1_Channel6_IRQHandler() {
    DMA_Cmd(DMA1_Channel6, DISABLE);
    TIM_Cmd(TIM3, DISABLE);
	DMA_ClearFlag(DMA1_FLAG_TC6);
	NVIC_ClearPendingIRQ(DMA1_Channel6_IRQn);
	transferRunning = false;
}

