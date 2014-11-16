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

static uint16_t stripe[LED][3];

int main(void) {

    // clear stripe
    for(uint16_t i = 0; i < LED; i++){
        stripe[i][0] = 0;
        stripe[i][1] = 0;
        stripe[i][2] = 0;
    }


    WS2812_Init();

    WS2812_send(stripe, 60);

    Delay(5000000L);

    while(1){
        for(uint16_t j = 0; j < LED; j++){
            // wandering light
            for(uint16_t i = 0; i < LED; i++){
                stripe[i][0] = 0;
                stripe[i][1] = 0;
                stripe[i][2] = 0;
            }
            stripe[j][0] = 0;
            stripe[j][1] = 0;
            stripe[j][2] = 255;

            WS2812_send(stripe, LED);
            Delay(500000L);
        }
    }




    //stripe[1][1] = 255;

    //WS2812_send(stripe, LED);

    while(1){
        Delay(500000L);
    }
}
