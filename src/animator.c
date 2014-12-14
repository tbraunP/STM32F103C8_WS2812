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


// Visulization
// 0..100
#define SATURATION                  (70)
// 0..360
#define BASECOLOR_SECONDS           (100)
#define BASECOLOR_MINUTES           (300)
#define BASECOLOR_HOURS             (200)
// 0..100
#define MAX_BRIGHTNESS              (10)

// macrotick counter
static volatile uint16_t loops = 0;
static volatile uint16_t seconds = 0;
static volatile uint16_t minutes = 0;
static volatile uint16_t hours = 0;

/**
 * data structure for animation (2 leds + brightness)
 * led - which should be light up;
 * lightUp - 0 <= lightUp <= 1 (intensity);
 * ledNext - nextLed which should be light up;
 * lightUpNext - 0 <= lightUp <= 1 (intensity)
 */
typedef struct ANIM_LED_t{
    uint8_t led, ledNext;
    float lightUpLED, lightUpLEDNext;
} ANIM_LED_t;

// forward declarations
static void updateVisualization(uint16_t hours, uint16_t minutes, uint16_t seconds, uint16_t posInSecond);

// RGB structure to draw on
static RGB_T rgbStripe[LED];
static HSV_T hsvStripe[LED];


void Animator_Init(){

    // start WS2812
    WS2812_Init();
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
        NVIC_ClearPendingIRQ(TIM4_IRQn);

        // now increment local loop counter and modify local
        // time on second increment
        ++loops;


        if(loops % UPDATE_RATE_SEC == 0){
            // primitve watch
            seconds++;
            if(seconds == 60){
                seconds = 0;
                ++minutes;
                if(minutes == 60){
                    ++hours;
                    minutes = 0;
                    if(hours == 24){
                        hours = 0;
                    }
                }
            }


            seconds = seconds % 60;
            loops = 0;
            //UART_SendString("S\n\0");
        }

        // update timer compare register and update duration of current cycle /second in ticks
        TIM4->CCR1 += COUNTERVALUE40MS;

        // update visualization of clock
        updateVisualization(hours, minutes, seconds, (loops % UPDATE_RATE_SEC));
    }
}

/**
 * @brief calcLED
 * Calculate which LED in ring should be light up. Therefore, totalAnimationSteps is the number of executions of this method,
 * upon the LED should move arround the whole ring. 0<= relPosition < totalAnimationSteps encodes the current position within
 * the ring.
 *
 * @param totalAnimationSteps - number of animationsteps until LED should surround the ring
 * @param relPosition -  position within ring 0<= relPosition < totalAnimationSteps
 * @param animation - animation entry (led which should be light up; lightUp - 0 <= lightUp <= 1 (intensity); ledNext - nextLed which should be light up;lightUpNext - 0 <= lightUp <= 1 (intensity)
 */
static void calcLED(uint32_t totalAnimationSteps, uint32_t relPosition, ANIM_LED_t* animation){
    // calculate degree, if one roations consists of anisteps
    float deg = (360.0 * relPosition) / totalAnimationSteps;
    deg = ((deg >= 360) ? (deg - 360.0) : deg);

    // degree per LED
    // 1 LED at 0
    // 2 LED at 1*dst
    // 3 LED at 2*dest etc.
    float dst = 360.0 / LED;

    // calculate indize of leds
    animation->led = (uint32_t) deg/dst;
    animation->led = animation->led % LED;
    animation->ledNext = (animation->led + 1) % LED;

    // ligh up value
    animation->lightUpLED = 1-(deg/dst - animation->led);
    animation->lightUpLEDNext = 1 - animation->lightUpLED;
}


static void updateVisualization(uint16_t hours, uint16_t minutes, uint16_t seconds, uint16_t posInSecond){
    uint32_t aniSteps = RATE_MIN;
    uint32_t clk = seconds * UPDATE_RATE_SEC + posInSecond;


    // Sekundenanimation
    ANIM_LED_t secondAnimation;
    calcLED(aniSteps, clk, &secondAnimation);

    // Minuten-Darstellung
    ANIM_LED_t minutesAnimation;
    aniSteps = RATE_MIN * 60;
    //clk = minutes * RATE_MIN + seconds * UPDATE_RATE_SEC + posInSecond;
    clk = minutes * RATE_MIN + clk;
    calcLED(aniSteps, clk, &minutesAnimation);

    // Stundenanimation
    ANIM_LED_t hoursAnimation;
    aniSteps = RATE_MIN * 60 * 12; // 12 hours display
    clk = (hours % 12) * RATE_MIN * 60 * 12 + clk;
    calcLED(aniSteps, clk, &hoursAnimation);


    // now set output
    for(uint32_t i = 0; i < LED; i++){
        hsvStripe[i].s = SATURATION;
        hsvStripe[i].h = 100;
        hsvStripe[i].v = 0;
    }

    // not set lightup
    hsvStripe[secondAnimation.led].v = MAX_BRIGHTNESS * secondAnimation.lightUpLED;
    hsvStripe[secondAnimation.ledNext].v = MAX_BRIGHTNESS * secondAnimation.lightUpLEDNext;


    // minutes
    hsvStripe[minutesAnimation.led].h = 300;
    hsvStripe[minutesAnimation.ledNext].h = 300;
    hsvStripe[minutesAnimation.led].v = MAX_BRIGHTNESS * minutesAnimation.lightUpLED;
    hsvStripe[minutesAnimation.ledNext].v = MAX_BRIGHTNESS * minutesAnimation.lightUpLEDNext;

    // hours
    hsvStripe[hoursAnimation.led].h = 200;
    hsvStripe[hoursAnimation.ledNext].h = 200;
    hsvStripe[hoursAnimation.led].v = MAX_BRIGHTNESS * hoursAnimation.lightUpLED;
    hsvStripe[hoursAnimation.ledNext].v = MAX_BRIGHTNESS * hoursAnimation.lightUpLEDNext;



    //hsvStripe[led].v = 40.0 * lightUpLED;
    //hsvStripe[ledNext].v = 40.0 * lightUpLEDNext;


    // now convert to rgb value
    for(uint32_t i = 0; i < LED; i++){
        rgbStripe[i] = convertHSV2RGB(&hsvStripe[i]);
    }


    // no draw the fuck hahahahahaha
    WS2812_send(rgbStripe, LED);
}
