/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Josuah Demangeon <me@josuah.net>
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

#include "py/obj.h"
#include "py/objarray.h"
#include "py/runtime.h"

#include "nrfx_log.h"
#include "nrf_soc.h"
#include "nrfx_reset_reason.h"

#include "driver_board.h"
#include "driver_ov5640.h"
#include "driver_ecx336cn.h"
#include "driver_max77654.h"
#include "machine.h"

enum {
    RESET_BOOTUP,
    RESET_SOFTWARE,
    RESET_OTHER
};

STATIC mp_obj_t machine_power_hibernate(void)
{
    return mp_const_notimplemented;
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_power_hibernate_obj, &machine_power_hibernate);

NORETURN STATIC mp_obj_t machine_power_reset(void)
{
    board_deinit();
    NVIC_SystemReset();
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_power_reset_obj, &machine_power_reset);

STATIC mp_obj_t machine_power_reset_cause(void)
{
    uint32_t cause;

    cause = nrfx_reset_reason_get();
    if (cause & NRFX_RESET_REASON_SREQ_MASK)
        return MP_OBJ_NEW_SMALL_INT(RESET_SOFTWARE);
    if (cause == 0)
        return MP_OBJ_NEW_SMALL_INT(RESET_BOOTUP);
    return MP_OBJ_NEW_SMALL_INT(RESET_OTHER);
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_power_reset_cause_obj, &machine_power_reset_cause);

STATIC mp_obj_t machine_power_shutdown(mp_obj_t timeout)
{
    (void)timeout;
    return mp_const_notimplemented;
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_power_shutdown_obj, &machine_power_shutdown);

STATIC mp_obj_t machine_power_camera_on(void)
{
    ov5640_pwr_on();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_power_camera_on_obj, &machine_power_camera_on);

STATIC mp_obj_t machine_power_camera_off(void)
{
    ov5640_pwr_sleep(); 
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_power_camera_off_obj, &machine_power_camera_off);

STATIC mp_obj_t machine_power_display_on(void)
{
    max77654_rail_10v(true);
    ecx336cn_awake(); 
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_power_display_on_obj, &machine_power_display_on);

STATIC mp_obj_t machine_power_display_off(void)
{
    ecx336cn_sleep();
    max77654_rail_10v(false);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_power_display_off_obj, &machine_power_display_off);

STATIC const mp_rom_map_elem_t machine_power_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_hibernate),   MP_ROM_PTR(&machine_power_hibernate_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset),       MP_ROM_PTR(&machine_power_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset_cause), MP_ROM_PTR(&machine_power_reset_cause_obj) },
    { MP_ROM_QSTR(MP_QSTR_display_on),  MP_ROM_PTR(&machine_power_display_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_display_off), MP_ROM_PTR(&machine_power_display_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_camera_on),   MP_ROM_PTR(&machine_power_camera_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_camera_off),  MP_ROM_PTR(&machine_power_camera_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_shutdown),    MP_ROM_PTR(&machine_power_shutdown_obj) },

    { MP_ROM_QSTR(MP_QSTR_POWER_RESET_BOOTUP),   MP_OBJ_NEW_SMALL_INT(RESET_BOOTUP) },
    { MP_ROM_QSTR(MP_QSTR_POWER_RESET_SOFTWARE), MP_OBJ_NEW_SMALL_INT(RESET_SOFTWARE) },
    { MP_ROM_QSTR(MP_QSTR_POWER_RESET_OTHER),    MP_OBJ_NEW_SMALL_INT(RESET_OTHER) },
};
STATIC MP_DEFINE_CONST_DICT(machine_power_locals_dict, machine_power_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_power_type,
    MP_QSTR_Power,
    MP_TYPE_FLAG_NONE,
    locals_dict, &machine_power_locals_dict
);
