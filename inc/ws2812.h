/*
 * ws2812.h
 *
 *  Created on: 19.06.2014
 *      Author: pyro
 */

#ifndef WS2812_H_
#define WS2812_H_
#include <stdint.h>
#include "colors.h"


#ifdef __cplusplus
 extern "C" {
#endif

#define LED (61)

void WS2812_Init();

void WS2812_send(RGB_T* color, uint16_t leds);

void DMA1_Channel6_IRQHandler();


#ifdef __cplusplus
 }
#endif

#endif /* WS2812_H_ */
