#ifndef _MATH_H_
#define _MATH_H_

#include <stdint.h>

// Definition of absolute value of entered number
#define abs(x) ((x) < 0 ? (-x) : (x))

// Definition of max value funtion of pair of numbers
#define max(x, y) ((x) > (y) ? (x) : (y))

// Definition of min value funtion of pair of numbers
#define min(x, y) ((x) < (y) ? (x) : (y))

uint32_t pow(uint32_t x, uint32_t y);
/*
TODO: Implement
uint32_t sqrt(uint32_t x);
uint32_t ceil(uint32_t x);
uint32_t floor(uint32_t x);
*/

#endif