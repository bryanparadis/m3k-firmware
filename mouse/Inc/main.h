/*
 * main.h
 *
 *  Created on: Mar 22, 2025
 *      Author: Bryan
 */

#ifndef MAIN_H_
#define MAIN_H_

typedef union {
	struct __PACKED { // use the order in the report descriptor
		uint8_t btn;
		int8_t whl;
		int16_t x, y;
		uint16_t _pad; // zero pad to 8 bytes total
	};
	uint8_t u8[8]; // btn, wheel, xlo, xhi, ylo, yhi, 0, 0
	uint32_t u32[2];
} Usb_packet;
//static_assert(sizeof(Usb_packet) == 2*sizeof(uint32_t), "Usb_packet wrong size");

extern volatile Usb_packet packet;
//extern volatile Usb_packet last;
/*
extern volatile uint8_t ready;
extern volatile int8_t skip;
extern volatile uint8_t hs_usb;
extern volatile uint8_t sync;
*/
#endif /* MAIN_H_ */
