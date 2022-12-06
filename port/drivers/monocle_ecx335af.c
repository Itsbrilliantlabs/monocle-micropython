/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

/**
 * Sony Microdisplay driver.
 * @file monocle_ecx335af.c
 * @author Shreyas Hemachandra
 * @author Nathan Ashelman
 */

#include "monocle_ecx335af.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "monocle_board.h"
#include "monocle_config.h"
#include "monocle_spi.h"
#include "nrf_gpio.h"
#include "nrfx_log.h"
#include "nrfx_systick.h"

static inline const void ecx335af_write_byte(uint8_t addr, uint8_t data)
{
    uint8_t buf[2] = { addr, data };

    spi_chip_select(SPIM0_DISP_CS_PIN);
    spi_xfer(buf, sizeof buf);
    spi_chip_deselect(SPIM0_DISP_CS_PIN);
}

static inline uint8_t ecx335af_read_byte(uint8_t addr)
{
    uint8_t buf[2] = { 0x81, 0x00 };

    ecx335af_write_byte(0x80, 0x01);
    ecx335af_write_byte(0x81, addr);

    spi_chip_select(SPIM0_DISP_CS_PIN);
    spi_xfer(buf, sizeof buf);
    spi_chip_deselect(SPIM0_DISP_CS_PIN);

    return buf[1];
}

#define LOG(...) NRFX_LOG_ERROR(__VA_ARGS__)

/**
 * Prepare GPIO pins before the chip receives power.
 */
void ecx335af_prepare(void)
{
    // Set to 0V on boot (datasheet p.11)
    nrf_gpio_pin_write(ECX335AF_XCLR_PIN, 0);
    nrf_gpio_cfg_output(ECX335AF_XCLR_PIN);
}

/**
 * Configure each value of the screen over SPI.
 */
void ecx335af_init(void)
{
    // power-on sequence, see Datasheet section 9
    // 1ms after 1.8V on, device has finished initializing
    nrfx_systick_delay_ms(1);

    // set XCLR to high (1.8V to take it) to change to power-saving mode
    nrf_gpio_pin_set(ECX335AF_XCLR_PIN);
    // SONY ECX336-CN register configuration, see Datasheet section 10.1
    // for RGB mode
    //ecx335af_write_byte(0x00, 0x0E); // [0]=0 -> enter power save mode
    //ecx335af_write_byte(0x01, 0x00);
    // for YUV mode
    ecx335af_write_byte(0x00, 0x9E); // [0]=0 -> enter power save mode
    ecx335af_write_byte(0x01, 0x20);
    // for register 0x00, also change the last line of this function

    ecx335af_write_byte(0x02, 0x00);
    ecx335af_write_byte(0x03, 0x20);  // 1125  
    ecx335af_write_byte(0x04, 0x3F);
    ecx335af_write_byte(0x05, 0xC8);  // 1125  DITHERON, LUMINANCE=0x00=2000cd/m2=medium (Datasheet 10.8)
    ecx335af_write_byte(0x06, 0x00);
    ecx335af_write_byte(0x07, 0x40);
    ecx335af_write_byte(0x08, 0x80);  // Luminance adjustment: OTPCALDAC_REGDIS=0 (preset mode per reg 5), white chromaticity: OTPDG_REGDIS=0 (preset mode, default)
    ecx335af_write_byte(0x09, 0x00);
    ecx335af_write_byte(0x0A, 0x10);
    ecx335af_write_byte(0x0B, 0x00);
    ecx335af_write_byte(0x0C, 0x00);
    ecx335af_write_byte(0x0D, 0x00);
    ecx335af_write_byte(0x0E, 0x00);
    ecx335af_write_byte(0x0F, 0x56);
    ecx335af_write_byte(0x10, 0x00);
    ecx335af_write_byte(0x11, 0x00);
    ecx335af_write_byte(0x12, 0x00);
    ecx335af_write_byte(0x13, 0x00);
    ecx335af_write_byte(0x14, 0x00);
    ecx335af_write_byte(0x15, 0x00);
    ecx335af_write_byte(0x16, 0x00);
    ecx335af_write_byte(0x17, 0x00);
    ecx335af_write_byte(0x18, 0x00);
    ecx335af_write_byte(0x19, 0x00);
    ecx335af_write_byte(0x1A, 0x00);
    ecx335af_write_byte(0x1B, 0x00);
    ecx335af_write_byte(0x1C, 0x00);
    ecx335af_write_byte(0x1D, 0x00);
    ecx335af_write_byte(0x1E, 0x00);
    ecx335af_write_byte(0x1F, 0x00);
    ecx335af_write_byte(0x20, 0x01);
    ecx335af_write_byte(0x21, 0x00);
    ecx335af_write_byte(0x22, 0x40);
    ecx335af_write_byte(0x23, 0x40);
    ecx335af_write_byte(0x24, 0x40);
    ecx335af_write_byte(0x25, 0x80);
    ecx335af_write_byte(0x26, 0x40);
    ecx335af_write_byte(0x27, 0x40);
    ecx335af_write_byte(0x28, 0x40);
    ecx335af_write_byte(0x29, 0x0B);
    ecx335af_write_byte(0x2A, 0xBE);    // CALDAC=190 (ignored, since OTPCALDAC_REGDIS=0)
    ecx335af_write_byte(0x2B, 0x3C);
    ecx335af_write_byte(0x2C, 0x02);
    ecx335af_write_byte(0x2D, 0x7A);
    ecx335af_write_byte(0x2E, 0x02);
    ecx335af_write_byte(0x2F, 0xFA);
    ecx335af_write_byte(0x30, 0x26);
    ecx335af_write_byte(0x31, 0x01);
    ecx335af_write_byte(0x32, 0xB6);
    ecx335af_write_byte(0x33, 0x00);
    ecx335af_write_byte(0x34, 0x03);
    ecx335af_write_byte(0x35, 0x60);    // 1125
    ecx335af_write_byte(0x36, 0x00);
    ecx335af_write_byte(0x37, 0x76);
    ecx335af_write_byte(0x38, 0x02);
    ecx335af_write_byte(0x39, 0xFE);
    ecx335af_write_byte(0x3A, 0x02);
    ecx335af_write_byte(0x3B, 0x71);    // 1125
    ecx335af_write_byte(0x3C, 0x00);
    ecx335af_write_byte(0x3D, 0x1B);
    ecx335af_write_byte(0x3E, 0x00);
    ecx335af_write_byte(0x3F, 0x1C);
    ecx335af_write_byte(0x40, 0x02);    // 1125
    ecx335af_write_byte(0x41, 0x4D);    // 1125
    ecx335af_write_byte(0x42, 0x02);    // 1125
    ecx335af_write_byte(0x43, 0x4E);    // 1125
    ecx335af_write_byte(0x44, 0x80);
    ecx335af_write_byte(0x45, 0x00);
    ecx335af_write_byte(0x46, 0x00);
    ecx335af_write_byte(0x47, 0x2D);    // 1125
    ecx335af_write_byte(0x48, 0x08);
    ecx335af_write_byte(0x49, 0x01);    // 1125
    ecx335af_write_byte(0x4A, 0x7E);    // 1125
    ecx335af_write_byte(0x4B, 0x08);
    ecx335af_write_byte(0x4C, 0x0A);    // 1125
    ecx335af_write_byte(0x4D, 0x04);    // 1125
    ecx335af_write_byte(0x4E, 0x00);
    ecx335af_write_byte(0x4F, 0x3A);    // 1125
    ecx335af_write_byte(0x50, 0x01);    // 1125
    ecx335af_write_byte(0x51, 0x58);    // 1125
    ecx335af_write_byte(0x52, 0x01);   
    ecx335af_write_byte(0x53, 0x2D);
    ecx335af_write_byte(0x54, 0x01);
    ecx335af_write_byte(0x55, 0x15);    // 1125
    ecx335af_write_byte(0x56, 0x00);
    ecx335af_write_byte(0x57, 0x2B);
    ecx335af_write_byte(0x58, 0x11);    // 1125
    ecx335af_write_byte(0x59, 0x02);
    ecx335af_write_byte(0x5A, 0x11);    // 1125
    ecx335af_write_byte(0x5B, 0x02);  
    ecx335af_write_byte(0x5C, 0x25);
    ecx335af_write_byte(0x5D, 0x04);    // 1125
    ecx335af_write_byte(0x5E, 0x0B);    // 1125
    ecx335af_write_byte(0x5F, 0x00);
    ecx335af_write_byte(0x60, 0x23);
    ecx335af_write_byte(0x61, 0x02);
    ecx335af_write_byte(0x62, 0x1A);    // 1125
    ecx335af_write_byte(0x63, 0x00);
    ecx335af_write_byte(0x64, 0x0A);    // 1125
    ecx335af_write_byte(0x65, 0x01);    // 1125
    ecx335af_write_byte(0x66, 0x8C);    // 1125
    ecx335af_write_byte(0x67, 0x30);    // 1125
    ecx335af_write_byte(0x68, 0x00);   
    ecx335af_write_byte(0x69, 0x00);    // 1125
    ecx335af_write_byte(0x6A, 0x00);
    ecx335af_write_byte(0x6B, 0x00);
    ecx335af_write_byte(0x6C, 0x00);
    ecx335af_write_byte(0x6D, 0x00);    // 1125
    ecx335af_write_byte(0x6E, 0x00);
    ecx335af_write_byte(0x6F, 0x60);
    ecx335af_write_byte(0x70, 0x00);
    ecx335af_write_byte(0x71, 0x00);
    ecx335af_write_byte(0x72, 0x00);
    ecx335af_write_byte(0x73, 0x00);
    ecx335af_write_byte(0x74, 0x00);
    ecx335af_write_byte(0x75, 0x00);
    ecx335af_write_byte(0x76, 0x00);
    ecx335af_write_byte(0x77, 0x00);
    ecx335af_write_byte(0x78, 0x00);
    ecx335af_write_byte(0x79, 0x68);
    ecx335af_write_byte(0x7A, 0x00);
    ecx335af_write_byte(0x7B, 0x00);
    ecx335af_write_byte(0x7C, 0x00);
    ecx335af_write_byte(0x7D, 0x00);
    ecx335af_write_byte(0x7E, 0x00);
    ecx335af_write_byte(0x7F, 0x00);

    //ecx335af_write_byte(0x00, 0x0F); // exit power saving mode, RGB
    ecx335af_write_byte(0x00, 0x9F); // exit power saving mode, YUV

    nrfx_systick_delay_ms(1);

    // check that 0x29 changed from default 0x0A to 0x0B
    // and that 0x2A has been restored
    assert(ecx335af_read_byte(0x29) == 0x0B);
    assert(ecx335af_read_byte(0x2A) == 0xBE);
}

void ecx335af_deinit(void)
{
    nrf_gpio_cfg_default(SPIM0_DISP_CS_PIN);
    nrf_gpio_cfg_default(ECX335AF_XCLR_PIN);
}

/**
 * Configure the luminance level of the display.
 * @param level Predefined level of luminance.
 */
void ecx335af_set_luminance(ecx335af_luminance_t level)
{
    uint8_t prev_0x05, new_0x05, check_0x05;  // register values

    // maximum value value is 4
    if (level > 4) return; 
    // LUMINANCE is register 0x05[3:0]; preserve other bits
    prev_0x05 = ecx335af_read_byte(0x05);
    new_0x05 = prev_0x05 & 0xF8;         // clear lower 3 bits
    new_0x05 = new_0x05 | level;
    // write new value
    ecx335af_write_byte(0x05, new_0x05);
    check_0x05 = ecx335af_read_byte(0x05);
    assert(check_0x05 == new_0x05);
}

/**
 * Put the display to sleep power mode.
 */
void ecx335af_sleep(void)
{
    ecx335af_write_byte(0x00, 0x9E); // enter power saving mode (YUV)
    // it is now possible to turn off the 10V rail
}

/**
 * Power back on the dispay from sleep mode.
 */
void ecx335af_awake(void)
{
    // the 10V power rail needs to be turned back on first
    ecx335af_write_byte(0x00, 0x9F); // exit power saving mode (YUV)
}
