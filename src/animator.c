#include "animator.h"

#include <stdio.h>

#include "stm32f10x_conf.h"
#include "stm32f10x.h"

#include "hw/uart.h"
#include "ws2812.h"


// CONSTANTS
#define     UPDATE_RATE_SEC         (25)
#define     RATE_MIN                (60*UPDATE_RATE_SEC)
#define     PRESCALER               (uint16_t) (0x2C1E)
#define     COUNTERVALUE40MS        (uint16_t) (0xFF)
#define     COUNTERVALUE40MS_2      (uint16_t) (COUNTERVALUE40MS/2)

// macrotick counter
static volatile uint16_t loops = 0;
static volatile uint16_t seconds = 0;


// forward declarations
static void updateVisualization(uint16_t seconds, uint16_t posInSecond, uint16_t animationStepsPerSecond);

// RGB structure to draw on
static RGB_T rgbStripe[LED];
static HSV_T hsvStripe[LED];


void Animator_Init(){

    // start WS2812
    WS2812_Init();
    WS2812_clear();
    WS2812_clear();


    // Timer configuration
    TIM_TimeBaseInitTypeDef timerConfig;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    NVIC_InitTypeDef   NVIC_InitStructure;


    // now we need a timer to decide if we received a bit or not
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    TIM_DeInit(TIM4);
    TIM_TimeBaseStructInit(&timerConfig);

    /* Time base configuration */
    timerConfig.TIM_Period = 0xFFFF;
    timerConfig.TIM_Prescaler = PRESCALER-1;
    timerConfig.TIM_ClockDivision = 0;
    timerConfig.TIM_CounterMode = TIM_CounterMode_Up;

    TIM_TimeBaseInit(TIM4, &timerConfig);
    TIM_ITConfig(TIM4, TIM_IT_CC1, ENABLE);
    TIM_ClearITPendingBit(TIM4, TIM_IT_CC1);

    /* Compare interrupt */
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Active;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Disable;
    //TIM_OCInitStructure.TIM_Pulse = 0;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    //TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
    TIM_OC1Init(TIM4, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Disable);

    // set compare value
    TIM4->CCR1 = COUNTERVALUE40MS;

    // Enable Interrupt
    NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    // reset interrupt
    NVIC_ClearPendingIRQ(TIM4_IRQn);
    NVIC_Init(&NVIC_InitStructure);

    // now start the timer
    TIM_Cmd(TIM4, ENABLE);
}




/**
 * Compare Interrupt
 */
void TIM4_IRQHandler(void){
    if(TIM_GetITStatus(TIM4, TIM_IT_CC1) == SET){
        // clear IRQ Status
        TIM_ClearITPendingBit(TIM4, TIM_IT_CC1);
        NVIC_ClearPendingIRQ(TIM2_IRQn);

        // now increment local loop counter and modify local
        // time on second increment
        ++loops;
        if(loops == RATE_MIN){
            // reset timing information for next 1 second round
            loops = 0;
        }

        if(loops % UPDATE_RATE_SEC == 0){
            seconds++;
            seconds = seconds % 60;
            UART_SendString("S\n\0");
        }

        // update timer compare register and update duration of current cycle /second in ticks
        TIM4->CCR1 += COUNTERVALUE40MS;

        // update visualization of clock
        updateVisualization(seconds, (loops % UPDATE_RATE_SEC), UPDATE_RATE_SEC);
        // clear IRQ Status
        TIM_ClearITPendingBit(TIM4, TIM_IT_CC1);
        NVIC_ClearPendingIRQ(TIM2_IRQn);
    }
}


static void updateVisualization(uint16_t seconds, uint16_t posInSecond, uint16_t animationStepsPerSecond){
    uint32_t aniSteps = RATE_MIN;
    uint32_t clk = seconds * UPDATE_RATE_SEC + posInSecond;

    // degree if one roations consists of anisteps
    float deg = (360.0 * clk) / aniSteps;
    deg = ((deg >= 360) ? (deg - 360.0) : deg);

    // degree per LED
    // 1 LED at 0
    // 2 LED at 1*dst
    // 3 LED at 2*dest etc.
    float dst = 360.0 / LED;

    // calculate indize of leds
    uint32_t led = (uint32_t) deg/dst;
    led = led % LED;
    uint32_t ledNext = (led +1) % LED;

    // ligh up value
    float lightUpLED = 1-(deg/dst - led);
    float lightUpLEDNext = 1- lightUpLED;

//    // now set output
//    for(uint32_t i = 0; i < LED; i++){
//        hsvStripe[i].s = 100;
//        hsvStripe[i].h = 250;
//        hsvStripe[i].v = 0;
//    }

//    // not set lightup
//    hsvStripe[led].v = 100.0 * lightUpLED;
//    hsvStripe[ledNext].v = 100.0 * lightUpLEDNext;

//    // now convert to rgb value
//    for(uint32_t i = 0; i < LED; i++){
//        rgbStripe[i] = convertHSV2RGB(&hsvStripe[i]);
//    }

    for(uint32_t i = 0; i < LED; i++){
        rgbStripe[i].blue = 0;
        rgbStripe[i].green = 0;
        rgbStripe[i].red = 0;
    }


    //lightUpLED  = 1.0;
    //lightUpLEDNext = 1.0;

    rgbStripe[led].green = 255.0 * lightUpLED;
    rgbStripe[led].red = 0 * lightUpLED;
    rgbStripe[led].blue = 0 * lightUpLED;

//    rgbStripe[ledNext].green = 255.0 * lightUpLEDNext;
//    rgbStripe[ledNext].red = 0 * lightUpLEDNext;
//    rgbStripe[ledNext].blue = 0 * lightUpLEDNext;


    // no draw the fuck hahahahahaha
    WS2812_send(rgbStripe, LED);
}
