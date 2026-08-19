#include "config.h"
#include "stm32f7xx.h"

// use flash sector 1, the 2nd 16kb (0x4000) sector
#define CONFIG_SECTOR_NUM  1
#define CONFIG_SECTOR_BASE (FLASHAXI_BASE + CONFIG_SECTOR_NUM*0x4000) // 0x08004000
#define CONFIG_SECTOR      ((__IO uint32_t *)CONFIG_SECTOR_BASE)
#define CONFIG_SECTOR_SIZE (0x4000 * sizeof(uint8_t)/sizeof(uint64_t))

static int config_index = -1; // set on first call to read_config

#ifdef BOARD_M2K
    const Config config_default = (
            (0 & CONFIG_SWAP_LMB_AND_RMB) |
            (0 & CONFIG_ANGLE_SNAP_ON) |
            CONFIG_HS_USB | // HS USB
            0 << CONFIG_INTERVAL_Pos | // 8kHz
            (2 << CONFIG_LOD_Pos) | // 2mm LOD 0b10
            ((800/100 - 1) & CONFIG_DPI_Msk) // 800 dpi
    );
#elif BOARD_M3K
    const Config config_default = (
            (0 & CONFIG_SWAP_LMB_AND_RMB) |
            (0 & CONFIG_ANGLE_SNAP_ON) |
            CONFIG_HS_USB | // HS USB
            0 << CONFIG_INTERVAL_Pos | // 8kHz
            (1 << CONFIG_LOD_Pos) | // 2mm LOD 0b01
            ((800/50 - 1) & CONFIG_DPI_Msk) // 800 dpi
    );
#endif

static void flash_unlock(void)
{
    FLASH->KEYR = 0x45670123; // ref manual pg 71
    FLASH->KEYR = 0xCDEF89AB;
}

static void flash_lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

static void flash_busy_wait(void)
{
    while ((FLASH->SR & FLASH_SR_BSY) != 0);
}

static __attribute__((unused)) void flash_prog_u16(__IO uint16_t *addr, const uint16_t data)
{
    flash_busy_wait();
    MODIFY_REG(FLASH->CR,
            FLASH_CR_PSIZE,
            _VAL2FLD(FLASH_CR_PSIZE, 0b01) | FLASH_CR_PG); // 0b01 for 16-bit
    *addr = data;
    __DSB();
    flash_busy_wait();
    FLASH->CR &= ~FLASH_CR_PG;
}

static void flash_prog_u32(__IO uint32_t *addr, const uint32_t data)
{
    flash_busy_wait();
    MODIFY_REG(FLASH->CR,
            FLASH_CR_PSIZE,
            _VAL2FLD(FLASH_CR_PSIZE, 0b10) | FLASH_CR_PG); // 0b10 for 32-bit
    *addr = data;
    __DSB();
    flash_busy_wait();
    FLASH->CR &= ~FLASH_CR_PG;
}

// 64-bit requires >=2.7 VDD and VPP = ~8V. We don't have VPP so we write 32 bits twice
static void flash_prog_u64(__IO uint32_t *addr, const uint64_t data)
{
    flash_prog_u32(addr + 0, (uint32_t)(data >>  0));
    flash_prog_u32(addr + 1, (uint32_t)(data >> 32));
}

static void flash_sector_erase(int sector)
{
    flash_busy_wait();
    // assume voltage range 2.7 - 3.6V for PSIZE (see ref manual pg 71)
    MODIFY_REG(FLASH->CR,
            FLASH_CR_PSIZE | FLASH_CR_SNB,
            _VAL2FLD(FLASH_CR_PSIZE, 0b10) | _VAL2FLD(FLASH_CR_SNB, sector) | FLASH_CR_SER);
    FLASH->CR |= FLASH_CR_STRT;
    __DSB();
    flash_busy_wait();
    FLASH->CR &= ~(FLASH_CR_SNB | FLASH_CR_SER);
}

// assumes all programmed bytes of a are before the empty bytes.
// returns index of highest programmed address (i.e. not 0xFFFF)
// or 0 if nothing is programmed yet
static int index_highest(const __IO uint32_t *a, const int len)
{
    int start = 0;
    int end = len;
    while (start + 1 < end) { // binary search
        int mid = (start + end)/2;

        uint32_t lo = a[2 * mid + 0];
        uint32_t hi = a[2 * mid + 1];

        if (lo != 0xFFFFFFFFu && hi != 0xFFFFFFFFu)
            start = mid;
        else
            end = mid;
    }
    return start;
}

Config config_read(void)
{
    if (config_index == -1) { // first call to function
        config_index = index_highest(CONFIG_SECTOR, CONFIG_SECTOR_SIZE);
        // write default cfg if sector is completely empty
        if (config_index == 0 && CONFIG_SECTOR[config_index * 2] == 0xFFFFFFFF) {
            flash_unlock();
            flash_prog_u64(&CONFIG_SECTOR[config_index * 2], config_default);
            flash_lock();
        }
    }
    return (Config){CONFIG_SECTOR[config_index * 2]};
}

void config_write(Config cfg)
{
    config_index++;
    flash_unlock();
    if (config_index == CONFIG_SECTOR_SIZE) {
        config_index = 0;
        flash_sector_erase(CONFIG_SECTOR_NUM);
    }
    flash_prog_u64(&CONFIG_SECTOR[config_index * 2], cfg);
    flash_lock();
}
