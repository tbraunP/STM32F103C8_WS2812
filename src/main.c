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
    for (uint32_t i; i < delay; i++){
        __asm__ volatile("NOP");
    }
}

uint8_t stripe[LED][3];

int main(void) {

    // clear stripe
    for(uint16_t i = 0; i < LED; i++){
        stripe[i][0] = 0;
        stripe[i][1] = 0;
        stripe[i][2] = 0;
    }


	WS2812_Init();
	
	Delay(50000L);
	
    //stripe[2][0] = 120;
    //stripe[2][1] = 120;
    //stripe[2][2] = 120;


    stripe[0][0] = 8;
    stripe[0][1] = 255;
    stripe[0][2] = 255;


    stripe[2][0] = 8;
    stripe[2][1] = 255;
    stripe[2][2] = 255;


    //stripe[1][1] = 255;

     WS2812_send(stripe, 3);

	while(1){
      Delay(500000L);
	}
}
