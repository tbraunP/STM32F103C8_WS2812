#include "stm32f10x.h"
#include "stm32f10x_it.h"
#include "stm32f10x_conf.h"
#include "system_stm32f10x.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ws2812.h"
#include "colors.h"
#include "hw/uart.h"
#include "animator.h"


void Delay(uint32_t delay) {
    for (uint32_t i=0; i < delay; i++){
        __asm__ volatile("NOP");
    }
}

static RGB_T stripe[LED];

int main(void) {

    volatile float zz = 360.0 * zz;

    // clear stripe
    for(uint16_t i = 0; i < LED; i++){
        stripe[i].blue = 0;
        stripe[i].green = 0;
        stripe[i].red = 0;
    }

    // run uart
    UART_init();

    UART_SendString("STM32F103WS2812 says hello\n\0");

    WS2812_Init();

    //WS2812_clear();

    Delay(5000000L);

   //Animator_Init();


    while(1){
        for(uint16_t j = 1; j < LED; j++){
            // wandering light
            for(uint16_t i = 0; i < LED; i++){
                stripe[i].blue = 0;
                stripe[i].green = 0;
                stripe[i].red = 0;
            }

            stripe[j].red = 0xAF;
            stripe[j].green = 0;
            stripe[j].blue = 0;

            // first led bugfix? pegel zu klein?
//            if(j!=0){
//                stripe[0].blue = 1;
//            }



            //WS2812_Init();

            WS2812_send(stripe, LED);


            Delay(7500000L);
            //while(1);
        }
    }
}
