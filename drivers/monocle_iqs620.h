/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */
#ifndef IQS620_H
#define IQS620_H
#include <stdint.h>
#include <stdbool.h>
#include "nrfx_twi.h"
/**
 * Driver for the IQS620 hall effect, proximity sensor.
 * https://www.azoteq.com/images/stories/pdf/iqs620_datasheet.pdf
 *
 * @defgroup iqs620
 * @{
 */

typedef unsigned pin_t;

typedef enum {
    IQS620_BUTTON_B0,
    IQS620_BUTTON_B1
} iqs620_button_t;

typedef enum {
    IQS620_BUTTON_UP,
    IQS620_BUTTON_PROX,
    IQS620_BUTTON_DOWN,
} iqs620_event_t;

typedef void (*iqs620_callback_t)(iqs620_button_t button, iqs620_event_t event);

void iqs620_init(void);
void iqs620_reset(void);
void iqs620_callback(iqs620_button_t, iqs620_event_t);
uint32_t iqs620_get_id(void);
uint16_t iqs620_get_button_status(void); // added to mimic cy8cmbr3 interfece
uint16_t iqs620_get_count(uint8_t channel);

/** @} */
#endif

