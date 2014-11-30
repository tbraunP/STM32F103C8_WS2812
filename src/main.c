#include "stm32f10x.h"
#include "stm32f10x_it.h"
#include "stm32f10x_conf.h"
#include "system_stm32f10x.h"

#include "ws2812.h"
#include "colors.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "animator.h"


void Delay(uint32_t delay) {
    for (uint32_t i; i < delay; i++){
        __asm__ volatile("NOP");
    }
}

static RGB_T stripe[LED];
static RGB_T clear[LED];

int main(void) {

    // clear stripe
    for(uint16_t i = 0; i < LED; i++){
        stripe[i].blue = 0;
        stripe[i].green = 0;
        stripe[i].red = 0;
        clear[i].blue = 0;
        clear[i].red = 0;
        clear[i].green = 0;
    }


    WS2812_Init();

    WS2812_send(clear, LED);

    Delay(5000000L);

    Animator_Init();


    while(1){
        for(uint16_t j = 0; j < LED; j++){
            // wandering light
            for(uint16_t i = 0; i < LED; i++){
                stripe[i].blue = 0;
                stripe[i].green = 0;
                stripe[i].red = 0;
            }
            stripe[j].blue = 255;
            stripe[j].green = 255;
            stripe[j].red = 255;

            WS2812_send(clear, LED);
            WS2812_send(clear, LED);

            WS2812_send(stripe, LED);
            Delay(750000L);
        }
    }




    //stripe[1][1] = 255;

    //WS2812_send(stripe, LED);

    while(1){
        Delay(500000L);
    }
}
