#pragma once

#include <assert.h>
#include <stdint.h>
#include "cmsis_compiler.h"
// flags bits
//      |15       |14       |13     |12     |11     |10     |9      |8 ... 0|
// 0    |SWAP off |AS off   |FS USB |   Interval    |      LOD      |DPI    |
// 1    |SWAP on  |AS on    |HS USB |   Interval    |      LOD      |DPI    |

// USB report rate:
//          |FS USB |HS USB |
//  Interval+-------+-------|
//     0b00 |   1ms | 125us |
//     0b01 |   2ms | 250us |
//     0b10 |   4ms | 500us |
//     0b11 |   8ms |   1ms |

// LOD: 0x00=1mm, 0x01=2mm, 0x02=3mm (3399 datasheet pg 61)

#define CONFIG_SWAP_LMB_AND_RMB (1 << 15)
#define CONFIG_ANGLE_SNAP_ON    (1 << 14)
#define CONFIG_HS_USB           (1 << 13)
#define CONFIG_INTERVAL_Pos     11
#define CONFIG_INTERVAL_Msk     (0b11 << CONFIG_INTERVAL_Pos)
#define CONFIG_INTERVAL         CONFIG_INTERVAL_Msk
#define CONFIG_LOD_Pos          9
#define CONFIG_LOD_Msk          (0b11 << CONFIG_LOD_Pos)
#define CONFIG_LOD              CONFIG_LOD_Msk
#define CONFIG_DPI_Pos          0
#define CONFIG_DPI_Msk          (0x1FF << CONFIG_DPI_Pos)
#define CONFIG_DPI              CONFIG_DPI_Msk

typedef uint64_t Config;

extern const Config config_default;

Config config_read(void);
void config_write(Config cfg);
