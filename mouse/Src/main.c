/* MIT License
 *
 * Copyright (c) 2023 Zaunkoenig GmbH
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <assert.h>
#include <m3k_resource.h>
#include <paw3399.h>
#include <stdint.h>
#include "stm32f7xx.h"
#include "usbd_hid.h"
#include "usb.h"
#include "anim.h"
#include "btn_whl.h"
#include "clock.h"
#include "config.h"
#include "delay.h"
#include "main.h"
#include "feature_report.h"

#define TIMEOUT_SECS 5 // seconds of holding buttons for programming mode

// used in usb.c via extern in main.h
volatile Usb_packet packet = {{1,0,0,0,0}}; // packet in progress will be sent next
uint8_t volatile ready = 0;
uint8_t volatile sync = 0;
uint8_t count = 0;
Usb_packet last_packet = {{1,0,0,0,0}};
volatile Feature_report feature_report_1 = {0};
volatile uint8_t config_update = 0;

static Config config_boot(void) {
	// read button state on boot
	uint8_t btn_boot = 0;
	btn_boot |= (!(LMB_NO_PORT->IDR & LMB_NO_PIN)) << 0;
	btn_boot |= (!(RMB_NO_PORT->IDR & RMB_NO_PIN)) << 1;

	// update config depending on initial buttons
	delay_ms(25); // delay in case power bounces on boot
	Config cfg = config_read();
	switch (btn_boot) {
	case 0b01: // LMB pressed
		cfg ^= CONFIG_ANGLE_SNAP_ON;
		config_write(cfg);
		if (cfg & CONFIG_ANGLE_SNAP_ON)
			anim_cw(1);
		else
			anim_ccw(1);
		break;
	case 0b11: // LMB and RMB pressed
		cfg ^= CONFIG_HS_USB;
		config_write(cfg);
		if (cfg & CONFIG_HS_USB)
			anim_eight(1);
		else
			anim_one(1);
		break;
	}

	// systick is enabled only if RMB was held initially (see bootloader)
	if (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk) {
		cfg = config_default;
		config_write(cfg);
		anim_diag(1);
	}
	return cfg;
}

static inline uint32_t mode_process(Config *cfg, int *skip,
		const uint8_t btn, const uint8_t btn_prev, const uint8_t squal) {
	// mode 0: normal
	// mode 1: cpi programming
	// mode 2: LOD/Hz programming
	static uint32_t mode = 0;
	static uint32_t ticks = 0; // counter for programming mode timeout
	static uint32_t large_step = 0; // ignore releases of the other button for large dpi steps.

	const int hs = ((*cfg & CONFIG_HS_USB) != 0);
	// 40000 / ( interval + 1)
	// frames_to_skip 0 = 8000Hz = 40000/(0+1) = 40000 timeout ticks
	// frames_to_skip 1 = 4000Hz = 40000/(1+1) = 20000 timeout ticks
	// frames_to_skip 3 = 2000Hz = 40000/(3+1) = 10000 timeout ticks
	// frames_to_skip 7 = 1000Hz = 40000/(7+1) =  5000 timeout ticks
	const int timeout_ticks = TIMEOUT_SECS * (hs ? (8000 /(*skip + 1)) : 1000);
	//const int timeout_ticks = hs ? (40000 / (*skip + 1)) : 1000;

	// typically squal in 60s for lifted 3399.
	const int SQUAL_THRESH = 75;
	const int lifted = (squal < SQUAL_THRESH);

	const uint16_t dpi_min = 0x0000; // = 0   = 50dpi
	const uint16_t dpi_max = 0x018F; // = 399 = 20000dpi
	const uint16_t DPI_LARGE_JUMP = 10; // 10 * 50dpi = 500.
	uint16_t dpi = _FLD2VAL(CONFIG_DPI, *cfg);

	if (mode == 1) { // handle cpi mode
		const uint8_t released = (~btn) & btn_prev;
		if ((released & 0b01) != 0 && !lifted) { // LMB released
			if (btn & 0b10) { // if RMB is held
				if (dpi != dpi_min) {
					dpi = MAX(dpi - DPI_LARGE_JUMP, dpi_min);
					anim_lg_downup(1);
				}
				large_step = 1;
			} else if (!large_step) {
				if (dpi != dpi_min) {
					dpi--;
					anim_downup(1);
				}
			} else {
				large_step = 0;
			}
			*cfg = (*cfg & (~CONFIG_DPI_Msk)) | dpi;
			paw3399_set_dpi(dpi);
		}
		if ((released & 0b10) != 0 && !lifted) { // RMB released
			if (btn & 0b01) { // if LMB is held
				if (dpi != dpi_max) {
					dpi = MIN(dpi + DPI_LARGE_JUMP, dpi_max);
					anim_lg_updown(1);
				}
				large_step = 1;
			} else if (!large_step) {
				if (dpi != dpi_max) {
					dpi++;
					anim_updown(1);
				}
			} else {
				large_step = 0;
			}
			*cfg = (*cfg & (~CONFIG_DPI_Msk)) | dpi;
			paw3399_set_dpi(dpi);
		}
	} else if (mode == 2) { // handle LOD/Hz mode
		const uint8_t released = (~btn) & btn_prev;
		if ((released & 0b01) != 0 && !lifted) { // LMB released
			const int new_lod = (_FLD2VAL(CONFIG_LOD, *cfg) + 1) % 3;
			*cfg = (*cfg & (~CONFIG_LOD_Msk)) | (new_lod << CONFIG_LOD_Pos);
			anim_cw(1 + new_lod);
			paw3399_set_lod(new_lod);
		}
		if ((released & 0b10) != 0 && !lifted && hs) { // RMB released in HS mode
			// loops 8k (0b00) -> 1k (0b11) -> 2k (0b10) -> 4k (0b01) -> 8k
			const int new_itv = (_FLD2VAL(CONFIG_INTERVAL, *cfg) - 1) % 4;
			*cfg = (*cfg & (~CONFIG_INTERVAL_Msk)) | (new_itv << CONFIG_INTERVAL_Pos);
			*skip = (1 << new_itv) - 1;
			anim_set_scale(hs ? (8 /(*skip + 1)) : 1);
			anim_num(1 << (3 - new_itv));
		}
	}

	if (mode == 0) {
		if (lifted && btn == 0b011) {
			ticks++;
			if (ticks == timeout_ticks) {
				// show DPI
				// 10k steps (50 * 200)
                anim_updown_pause((dpi + 1) / 200);
                // 1k steps (50 * 20)
                anim_rightleft_pause(((dpi + 1) % 200) / 20);
                // 100 steps (50 * 2)
                anim_downup_pause((((dpi + 1) % 200) % 20) / 2);
                // 50 steps (50 * 1)
                anim_leftright_pause((((dpi + 1) % 200) % 20) % 2);
			}
			if (ticks == 2*timeout_ticks) {
				anim_cw(1 + _FLD2VAL(CONFIG_LOD, *cfg));
				if (hs) {
					anim_pause(500);
					anim_num(1 << (3 - _FLD2VAL(CONFIG_INTERVAL, *cfg)));
				}
			}
		} else if (ticks < timeout_ticks) {
			ticks = 0;
		} // else, ticks >= timeout_ticks, save the value and wait

		if (btn == 0 && ticks >= timeout_ticks) {
			if (ticks >= 2*timeout_ticks) {
				mode = 2;
			} else { // timeout_ticks <= times <= 2*timeout_ticks
				mode = 1;
			}
		}
	} else { // mode == 1 || mode == 2
		if (lifted && btn == 0b011) {
			ticks++;
			if (ticks == timeout_ticks) {
				if (mode == 1) {
					// show DPI
					// 10k steps (50 * 200)
                    anim_updown_pause((dpi + 1) / 200);
                    // 1k steps (50 * 20)
                    anim_rightleft_pause(((dpi + 1) % 200) / 20);
                    // 100 steps (50 * 2)
                    anim_downup_pause((((dpi + 1) % 200) % 20) / 2);
                    // 50 steps (50 * 1)
                    anim_leftright_pause((((dpi + 1) % 200) % 20) % 2);
				} else if (mode == 2) {
					anim_cw(1 + _FLD2VAL(CONFIG_LOD, *cfg));
					if (hs) {
						anim_pause(500);
						anim_num(1 << (3 - _FLD2VAL(CONFIG_INTERVAL, *cfg)));
					}
				}
				mode = 0;
				config_write(*cfg);
				feature_report_1.words[0] = cfg;
				ticks = 0;
			}
		} else {
			ticks = 0;
		}
	}

	// input mask
	return (mode == 0) ? btn : 0x00U;

}

int main(void) {
	extern uint32_t _sflash;
	SCB->VTOR = (uint32_t) (&_sflash);
	SCB_EnableICache();
	SCB_EnableDCache();

	clk_init();
	delay_init();
	btn_whl_init();

	// TODO not const maybe calls function each time
	Config cfg = config_boot();
	feature_report_1.words[0] = cfg;
	const int hs_usb = ((cfg & CONFIG_HS_USB) != 0);
	// 8000 to 1000, 2000, 4000
	// 0, 1, 2, 3
	// 0, 1, 3, 7 frames to skip
    int frames_to_skip = hs_usb ? (1 << _FLD2VAL(CONFIG_INTERVAL, cfg)) - 1 : 0;
	//int frames_to_skip = 0;
    int frame_counter = 0;
     // packet that was sent last

	usb_init(hs_usb);
	// frames_to_skip = 0 = 8000Hz = 8 / (0 + 1)  = 8 animation_scaling
	// frames_to_skip = 1 = 4000Hz = 8 / (1 + 1)  = 4 animation_scaling
	// frames_to_skip = 3 = 2000Hz = 8 / (3 + 1)  = 2 animation_scaling
	// frames_to_skip = 7 = 1000Hz = 8 / (7 + 1)  = 1 animation_scaling
	anim_set_scale(hs_usb ? (8 /(frames_to_skip + 1)) : 1);

	spi_init();
	paw3399_init(cfg);

	uint8_t btn_prev = 0;
	uint8_t btn_unmasked = 0;
	int whl_lastlast = whl_read();
	int whl_last = whl_lastlast;
	int whl_count = 0; // microframe counter for limiting wheel code rate

	usb_wait_configured();

	while (1) {

		if (config_update == 1){
			config_update = 0;

			config_write(feature_report_1.words[0]);

			// TODO set configuration live
		}

		// do not run until NAK or XFRC on EP1
		if (sync != 1)
			continue;
		sync = 0;

		// Testing delay by lifting mouse and holding button 1 while plugging in
		// If the first input report is actually sent on 2nd poll with 1 NAK before it is good


		// TODO add FS delay
		// TODO optimize and then adjust HS delay. Don't know possibly max loop length.
		//75us ok  1 poll
		//77us ok  1 poll
		//78us bad 2 polls
		//104us at 160MHz
		delay_us(60);

        // frames to skip 0 = 8000Hz, 1 = 4000Hz, 3 = 2000Hz and 7 = 1000Hz
		if (frames_to_skip != 0){
			if ( frame_counter == frames_to_skip) {
				frame_counter = 0;
				continue;
			} else if ( frame_counter != 0 ) {
				frame_counter++;
				continue;
			} else {
				frame_counter++;
			}
		}

		// reset packet
		// packet.btn = 0;  You need buffer btn data to stay for btn_prev
		packet.whl = 0;
		packet.x = 0;
		packet.y = 0;

		// read sensor, buttons
		ss_low();
		spi_send(0x16);
		delay_us(2);
		(void) spi_recv(); // motion, not used
		(void) spi_recv(); // observation, not used
		packet.u8[3] = spi_recv(); // x lower 8 bits
		packet.u8[4] = spi_recv(); // x upper 8 bits
		packet.u8[5] = spi_recv(); // y lower 8 bits
		packet.u8[6] = spi_recv(); // y upper 8 bits
		const uint8_t squal = spi_recv(); // SQUAL
		ss_high();

		if (whl_count == 0) {
			const int whl_now = whl_read();
			if (whl_now != whl_last) {
				if (!((whl_now == 0 && whl_last == 3) || (whl_now == 3 && whl_last == 0))) {
					if (whl_now == 0 && whl_lastlast == 3) {
						packet.whl = (whl_last == 1) ? -1 : (whl_last == 2) ? 1 : 0;
					} else if (whl_now == 3 && whl_lastlast == 0) {
						packet.whl = (whl_last == 1) ? 1 : (whl_last == 2) ? -1 : 0;
					}
					whl_lastlast = whl_last;
					whl_last = whl_now;
				}
			}
		}
		if (hs_usb) // only run wheel code every 4 microframes
			whl_count = (whl_count + 1) % 4;

		// TODO add button swap here buy swapping the left and right button bits
		const uint16_t btn_raw = btn_read();
		const uint8_t btn_NO = (btn_raw & 0xFF);
		const uint8_t btn_NC = (btn_raw >> 8);
		// TODO can this move to the end of the loop? It should replace btn_prev or something.
		btn_prev = btn_unmasked;
		// TODO is this necessary?
		// btn_unmasked saves the unprocessed button data to compare to next loop
		btn_unmasked = (~btn_NO & 0b11111) | (btn_NC & btn_prev);

		// mode processing returns btn or 0x00U if you are changing settings
	    packet.btn = mode_process(&cfg, &frames_to_skip, btn_unmasked, btn_prev, squal);

#if 1
		// animation stuff
		const struct Xy a = anim_read(); // returns 0 if no animation left
		packet.x += a.x;
		packet.y += a.y;
#endif

		// testing code that checks for dropping frames
#if 0
		if (count == 0) {
			packet.x = -100;
			packet.y = 0;
			count ++;
		} else {
			packet.x = 100;
		    packet.y = 0;
		    count = 0;
		}
#endif

#if 0
		// testing code that replaces btn with a counter 1-4 on each poll
		// counter starts at 0 but we want to do 1-4 on this one
		if (count == 0){
			count = 1;
		}

		packet.btn = count;
		if(count == 4){
			count = 1;
		} else {
			count++;
		}

#endif

		// TODO maybe critical section unnecessary now
		__disable_irq();
		// check to see if there is new data
		if ( packet.btn != last_packet.btn || packet.x || packet.y || packet.whl ) {
		  // save last packet
		  last_packet.btn = packet.btn;
		  ready = 1;
		  // enqueue fifo write
		  NVIC->STIR = OTG_HS_EP1_IN_IRQn;
		}
	    __enable_irq();
	} // while
	return 0;
}
