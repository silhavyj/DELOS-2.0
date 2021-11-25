#ifndef _LIMITS_H_
#define _LIMITS_H_

// Source: https://www.tutorialspoint.com/c_standard_library/limits_h.htm

#define CHAR_BIT    (8)             // Number of bits in byte
#define SCHAR_MIN   (-128)          // Min value for signed char
#define SCHAR_MAX   (127)           // Max value for signed char
#define UCHAR_MAX   (255)           // Max value for unsigned char
#define CHAR_MIN    (-128)          // minimum value for type char and its value will be equal to SCHAR_MIN if char represents negative values, otherwise zero
#define CHAR_MAX    (127)           // Value for type char and its value will be equal to SCHAR_MAX if char represents negative values, otherwise UCHAR_MAX
#define MB_LEN_MAX  (16)            // Max number of bytes in a multibyte character
#define SHRT_MIN    (-32768)        // Min value for short int
#define SHRT_MAX    (32767)         // Max value for short int
#define USHRT_MAX   (65535)         // Max value for unsigned short int
#define INT_MIN     (-2147483648)   // Min value for int
#define INT_MAX     (2147483647)    // Max value for int
#define UINT_MAX    (4294967295)    // Max value for unsigned int

#endif