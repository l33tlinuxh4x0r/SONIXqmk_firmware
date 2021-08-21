#include <ch.h>
#include <hal.h>

#include "print.h"
#include "eeprom_sn32.h"
#include <string.h>

/*************************************/
/*      SN32F240B EEPROM driver      */
/*                                   */
/*  Hacked together by l33linuxh4x0r */
/*        GPL Bla Bla Bla            */
/*************************************/


#ifndef EEPROM_SIZE
#   include "eeprom_sn32.h"
#   define EEPROM_SIZE 1024  // based off eeconfig's current usage, aligned to 4-byte sizes, to deal with LTO
#endif
__attribute__((aligned(4))) static uint8_t buffer[EEPROM_SIZE];


uint8_t DataBuf[FEE_PAGE_SIZE];

void EEPROM_Erase(void) {
    int page_num = 0;

    // delete all pages from specified start page to the last page
    do {
        FLASH_EraseSector(FEE_PAGE_BASE_ADDRESS + (page_num * FEE_PAGE_SIZE));
        page_num++;
    } while (page_num < FEE_DENSITY_PAGES);
}

uint16_t EEPROM_WriteDataByte(uint16_t Address, uint8_t DataByte) {
    uint32_t FlashStatus = FLASH_OKAY;

    uint32_t page;
    int      i;

    // exit if desired address is above the limit (e.G. under 2048 Bytes for 4 pages)
    if (Address > FEE_DENSITY_BYTES) {
        return 0;
    }

    // calculate which page is affected (Pagenum1/Pagenum2...PagenumN)
    page = FEE_ADDR_OFFSET(Address) / FEE_PAGE_SIZE;

    // if current data is 0xFF, the byte is empty, just overwrite with the new one
    if ((*(__IO uint16_t *)(FEE_PAGE_BASE_ADDRESS + FEE_ADDR_OFFSET(Address))) == FEE_EMPTY_WORD) {
        FlashStatus = FLASH_ProgramHalfWord(FEE_PAGE_BASE_ADDRESS + FEE_ADDR_OFFSET(Address), (uint8_t *)(0x00FF & DataByte));
        // FlashStatus = FLASH_ProgramPage(FEE_PAGE_BASE_ADDRESS + FEE_ADDR_OFFSET(Address), 2, (uint16_t)(0x00FF & DataByte));

    } else {
        // Copy Page to a buffer
        memcpy(DataBuf, (uint8_t *)FEE_PAGE_BASE_ADDRESS + (page * FEE_PAGE_SIZE), FEE_PAGE_SIZE);  // !!! Calculate base address for the desired page

        // check if new data is differ to current data, return if not, proceed if yes
        if (DataByte == *(__IO uint8_t *)(FEE_PAGE_BASE_ADDRESS + FEE_ADDR_OFFSET(Address))) {
            return 0;
        }

        // manipulate desired data byte in temp data array if new byte is differ to the current
        DataBuf[FEE_ADDR_OFFSET(Address) % FEE_PAGE_SIZE] = DataByte;

        // Erase Page
        FlashStatus = FLASH_EraseSector(FEE_PAGE_BASE_ADDRESS + (page * FEE_PAGE_SIZE));

        // __asm__ volatile ("bkpt");

        // Write new data (whole page) to flash if data has been changed
        for (i = 0; i < (FEE_PAGE_SIZE / 2); i++) {
            if ((__IO uint16_t)(0xFF00 | DataBuf[FEE_ADDR_OFFSET(i)]) != 0xFFFF) {
                FlashStatus = FLASH_ProgramHalfWord((FEE_PAGE_BASE_ADDRESS + (page * FEE_PAGE_SIZE)) + (i * 2), (uint8_t *)(0xFF00 | DataBuf[FEE_ADDR_OFFSET(i)]));
                // FlashStatus = FLASH_ProgramPage((FEE_PAGE_BASE_ADDRESS + (page * FEE_PAGE_SIZE)) + (i * 2), 2, (uint16_t)(0xFF00 | DataBuf[FEE_ADDR_OFFSET(i)]));
            }
        }
    }
    return FlashStatus;
}

uint8_t eeprom_read_byte(const uint8_t *addr) {
    uint32_t offset = (uint32_t)addr;
    uint8_t DataByte = 0xFF;
    DataByte = (*(__IO uint8_t *)(FEE_PAGE_BASE_ADDRESS + FEE_ADDR_OFFSET(offset)));
    uprintf("Data in EEPROM: %d \n", DataByte);
    uprintf("Data in RAM: %d \n", buffer[offset]);
    return DataByte;
}

void eeprom_write_byte(uint8_t *addr, uint8_t value) {
    print("eeporm writing byte...\n");
    uint32_t offset = (uint32_t)addr;
    buffer[offset]  = value;
    uprintf("Addr: %d ", offset);
    uprintf("Value: %d ", buffer[offset]);
    EEPROM_WriteDataByte((FEE_PAGE_BASE_ADDRESS + FEE_ADDR_OFFSET(offset)), buffer[offset]);
}

uint16_t eeprom_read_word(const uint16_t *addr) {
    print("eeporm writing word...\n");
    const uint8_t *p = (const uint8_t *)addr;
    return eeprom_read_byte(p) | (eeprom_read_byte(p + 1) << 8);
}

uint32_t eeprom_read_dword(const uint32_t *addr) {
    print("eeporm writing dword...\n");
    const uint8_t *p = (const uint8_t *)addr;
    return eeprom_read_byte(p) | (eeprom_read_byte(p + 1) << 8) | (eeprom_read_byte(p + 2) << 16) | (eeprom_read_byte(p + 3) << 24);
}

void eeprom_read_block(void *buf, const void *addr, size_t len) {
    print("eeporm reading block...\n");
    const uint8_t *p    = (const uint8_t *)addr;
    uint8_t *      dest = (uint8_t *)buf;
    while (len--) {
        *dest++ = eeprom_read_byte(p++);
    }
}

void eeprom_write_word(uint16_t *addr, uint16_t value) {
    print("eeporm writing word...\n");
    uint8_t *p = (uint8_t *)addr;
    eeprom_write_byte(p++, value);
    eeprom_write_byte(p, value >> 8);
}

void eeprom_write_dword(uint32_t *addr, uint32_t value) {
    print("eeporm writing dword...\n");
    uint8_t *p = (uint8_t *)addr;
    eeprom_write_byte(p++, value);
    eeprom_write_byte(p++, value >> 8);
    eeprom_write_byte(p++, value >> 16);
    eeprom_write_byte(p, value >> 24);
}

void eeprom_write_block(const void *buf, void *addr, size_t len) {
    print("eeporm writing block...\n");
    uint8_t *      p   = (uint8_t *)addr;
    const uint8_t *src = (const uint8_t *)buf;
    while (len--) {
        eeprom_write_byte(p++, *src++);
    }
}

/* chip selection */
// The update functions just calls write for now, but could probably be optimized

void eeprom_update_byte(uint8_t *addr, uint8_t value) { eeprom_write_byte(addr, value); }

void eeprom_update_word(uint16_t *addr, uint16_t value) {
    print("eeporm updating word...\n");
    uint8_t *p = (uint8_t *)addr;
    eeprom_write_byte(p++, value);
    eeprom_write_byte(p, value >> 8);
}

void eeprom_update_dword(uint32_t *addr, uint32_t value) {
    print("eeporm updating dword...\n");
    uint8_t *p = (uint8_t *)addr;
    eeprom_write_byte(p++, value);
    eeprom_write_byte(p++, value >> 8);
    eeprom_write_byte(p++, value >> 16);
    eeprom_write_byte(p, value >> 24);
}

void eeprom_update_block(const void *buf, void *addr, size_t len) {
    print("eeporm updating block...\n");
    uint8_t *      p   = (uint8_t *)addr;
    const uint8_t *src = (const uint8_t *)buf;
    while (len--) {
        eeprom_write_byte(p++, *src++);
    }
}
