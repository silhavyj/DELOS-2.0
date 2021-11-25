#ifndef _PIT_H_
#define _PIT_H_

// http://www.osdever.net/bkerndev/Docs/pit.htm

#define PIT0_DATA   0x40 // PIT0 data register
#define PIT_CMD     0x43 // PIT command register
#define FREQUENCY   100  // 100Hz

int PIT_init();

#endif