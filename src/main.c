#include "stm32f10x.h"
#include "stm32f10x_it.h"
#include "stm32f10x_conf.h"
#include "system_stm32f10x.h"

#include "ws2812.h"
#include "colors.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


void Delay(uint32_t delay) {
	for (uint32_t i; i < delay; i++)
		;
}

int main(void) {

	WS2812_Init();

	while (1) {
		/* first cycle through the colors on 2 LEDs chained together
		 * last LED in the chain will receive first sent triplet
		 * --> last LED in the chain will 'lead'
		 */
		for (uint16_t i = 0; i < 766; i += 2) {
			WS2812_send(&eightbit[i], 2);
			Delay(50000L);
		}

		/* cycle through the colors on only one LED
		 * this time only the first LED that data is
		 * fed into will update
		 */
		for (uint16_t i = 0; i < 766; i += 1) {
			WS2812_send(&eightbit[i], 1);
			Delay(50000L);
		}
	}
}
