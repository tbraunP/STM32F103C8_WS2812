/*
 * colors.h
 *
 *  Created on: 20.06.2014
 *      Author: pyro
 */

#ifndef COLORS_H_
#define COLORS_H_

#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

extern uint8_t eightbit[766][3];


typedef struct RGB_T{
    uint8_t red;
    uint8_t blue;
    uint8_t green;
} RGB_T;


#ifdef __cplusplus
 }
#endif

#endif /* COLORS_H_ */
