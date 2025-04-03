/*
 * feature_report.h
 *
 *  Created on: April 2, 2025
 *      Author: Bryan
 */

#ifndef FEATURE_REPORT_H_
#define FEATURE_REPORT_H_
#include <stdint.h>

/* Feature report buffer for Report ID 2 (32 x 16-bit = 64 bytes) */
typedef union {
    uint8_t bytes[32];   // Byte access for USB stack
    uint16_t words[16];  // 16-bit word access for application logic
} Feature_report;

extern volatile Feature_report feature_report_1; // Initialize to 0
extern volatile uint8_t config_update;

#endif /* FEATURE_REPORT_H_ */
