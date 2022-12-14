/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Josuah Demangeon <me@josuah.net>
 * Authored by: Nathan Ashelman <nathan@itsbrilliant.co>
 *
 * ISC Licence
 *
 * Copyright © 2022 Brilliant Labs Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>

#include "nrfx_saadc.h"
#include "nrf_gpio.h"
#include "nrfx_log.h"

#include "driver/battery.h"
#include "driver/config.h"
#include "driver/nrfx.h"
#include "driver/timer.h"

#define ASSERT  NRFX_ASSERT

/*
 * Lithium battery discharge curve, modeled from Grepow data for 1C discharge rate
 * Requirement: x-values (i.e. voltage) must be stricly increasing
 */

// Generated by tools/battery_discharge_curve.awk
static float battery_discharge_curve[10 + 1] = {
    3.80, 3.45, 3.18, 3.12, 3.10, 3.07, 3.02, 2.97, 2.89, 2.79, 2.70
};

// Depends on R_HI
// https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.nrf52832.ps.v1.1/saadc.html?cp=4_2_0_36_8#concept_qh3_spp_qr

// VDD=1.8V divided by 4 as reference
#define BATTERY_ADC_REFERENCE NRF_SAADC_REFERENCE_VDD4
#define REFERENCE (1.8 / 4.0)

// Resolution of the ADC: for a 10-bit ADC, the resolution is 1 << 10 = 1024
#define BATTERY_ADC_RESOLUTION NRF_SAADC_RESOLUTION_10BIT
#define RESOLUTION 10

// Gain 1/4, so input range = VDD (full range)
#define BATTERY_SAADC_GAIN_CONF NRF_SAADC_GAIN1_4
#define BATTERY_SAADC_GAIN (1.0 / 4.0)

// The V_AMUX goes to 1.25 V max when V_BATT goes to 4.5 V max
#define MAX77654_AMUX_GAIN (1.25 / 4.5)

// Total gain from the battery to the raw ADC result
#define GAIN (BATTERY_SAADC_GAIN * MAX77654_AMUX_GAIN)

// Stores battery state-of-charge, expressed in percent (0-100)
static uint8_t battery_percent;

// Reduces the frequency of battery sensing.
static uint8_t battery_timer_counter;

static float battery_saadc_to_voltage(nrf_saadc_value_t result)
{
    // V*GAIN / REFERENCE = RESULT / 2^RESOLUTION
    // V = RESULT / 2^RESOLUTION * REFERENCE / GAIN
    return (float)result / (float)(1<<RESOLUTION) * (float)REFERENCE / (float)GAIN;
}

/**
 * Interpolate the voltage level against the discharge curve to estimate the percentage.
 * @param voltage Voltage measured from the ADC.
 * @return Remaining battery percentage value [0..100].
 */
static uint8_t battery_voltage_to_percent(float voltage)
{
    // Above the max is considered 100%.
    if (voltage > battery_discharge_curve[0])
        return 100;

    // Search for the first value below the measured voltage.
    for (size_t i = 0; i < NRFX_ARRAY_SIZE(battery_discharge_curve); i++)
    {
        if (battery_discharge_curve[i] < voltage)
        {
            float v0 = battery_discharge_curve[i];
            float v1 = battery_discharge_curve[i - 1];
            float p0 = (10 - i) * 10;
            return round(p0 + 10 * (voltage - v0) / (v1 - v0));
        }
    }

    // Nothing below the voltage we measured, consider the battery at 0%.
    return 0;
}

/**
 * Get current, precomputed state-of-charge of the battery.
 * @return Remaining percentage of the battery.
 */
uint8_t battery_get_percent(void)
{
    // Everything is handled by the ADC callback above.
    return battery_percent;
}

/**
 * Compute the mean of the 10 last elements using the travelling mean method:
 * an incremental averaging that includes a weighted trace of the last N elements
 * in addition to the latest value. The max weight is 10: 1/10 of modifications max
 * every time this function is called.
 * @param new The new value to blend into the mean.
 * @return The newly computed mean value.
 */
float battery_travelling_mean(float new)
{
    static float mean = 0;
    static float count = 0;

    mean = (mean * count + new * 1) / (count + 1);
    count = (count == 10) ? (10) : (count + 1);
    return mean;
}

void battery_timer_handler(void)
{
    uint32_t err;
    nrf_saadc_value_t result;

    // Reduce the frequency at which this timer is called.
    if (++battery_timer_counter != 0) // letting it overflow
        return;

    // Configure first ADC channel with low setup (enough for battery sensing)
    err = nrfx_saadc_simple_mode_set(1u << 0, BATTERY_ADC_RESOLUTION,
        NRF_SAADC_OVERSAMPLE_DISABLED, NULL);
    ASSERT(err == NRFX_SUCCESS);

    // Add a buffer for the NRFX SDK to fill
    err = nrfx_saadc_buffer_set(&result, 1);
    ASSERT(err == NRFX_SUCCESS);

    // Start the trigger chain: the callback will trigger another callback
    err = nrfx_saadc_mode_trigger();
    NRFX_ASSERT(err == NRFX_SUCCESS);

    // Compute the voltage, then percentage
    float v_inst = battery_saadc_to_voltage(result);
    float v_mean = battery_travelling_mean(v_inst);
    battery_percent = battery_voltage_to_percent(v_mean);
    //LOG("%d", (int)(v_inst * 1000)); // for battery_discharge_curve.awk
    //LOG("v_inst=%d v_mean=%d %d%%", (int)(v_inst * 1000), (int)(v_mean * 1000), battery_percent);
}

/**
 * Initialize the ADC.
 * This includes setting up buffering.
 */
void battery_init(void)
{
    DRIVER("BATTERY");
    nrfx_init();

    uint32_t err;
    nrfx_saadc_channel_t channel = NRFX_SAADC_DEFAULT_CHANNEL_SE(BATTERY_ADC_PIN, 0);

    channel.channel_config.reference = BATTERY_ADC_REFERENCE;
    channel.channel_config.gain = BATTERY_SAADC_GAIN_CONF;

    nrf_gpio_cfg_input(BATTERY_ADC_PIN, NRF_GPIO_PIN_NOPULL);

    err = nrfx_saadc_channel_config(&channel);
    ASSERT(err == NRFX_SUCCESS);

    // Add a low-frequency house-cleaning timer
    timer_add_handler(&battery_timer_handler);
}
