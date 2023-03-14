/*
 * utils/init.c
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2021-2022 Rinnegatamante
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "utils/init.h"

#include "utils/dialog.h"
#include "utils/glutil.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/settings.h"

#include "dynlib.h"
#include "patch.h"

#include <malloc.h>
#include <string.h>

#include <psp2/appmgr.h>
#include <psp2/apputil.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/clib.h>
#include <psp2/power.h>
#include <psp2/touch.h>

#include <FalsoJNI/FalsoJNI.h>
#include <so_util/so_util.h>
#include <unzip/unzip.h>
#include <sys/syslimits.h>

// Base address for the Android .so to be loaded at
#define LOAD_ADDRESS 0xA0000000

// Weak symbols from AFakeNative to be overriden with our settings
float L_INNER_DEADZONE = 0.11f;
float R_INNER_DEADZONE = 0.11f;
int AInput_enableLeftStick = 1;
int AInput_enableRightStick = 0;

extern so_module so_mod;

void soloader_init_all() {
    // Set default overclock values
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

    // Enable analog stick and touchscreen
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, 1);

    if (!module_loaded("kubridge"))
        fatal_error("Error: kubridge.skprx is not installed.");
    log_info("kubridge check passed.");

    if (!file_exists(SO_PATH)) {
        fatal_error("Looks like you haven't installed the data files for this "
                    "port, or they are in an incorrect location. Please make "
                    "sure that you have %s file exactly at that path.", SO_PATH);
    }

    if (so_file_load(&so_mod, SO_PATH, LOAD_ADDRESS) < 0)
        fatal_error("Error: could not load %s.", SO_PATH);

    settings_load();
    L_INNER_DEADZONE = setting_leftStickDeadZone;
    R_INNER_DEADZONE = setting_rightStickDeadZone;
    log_info("settings_load() passed.");

    so_relocate(&so_mod);
    log_info("so_relocate() passed.");

    resolve_imports(&so_mod);
    log_info("so_resolve() passed.");

    so_patch();
    log_info("so_patch() passed.");

    so_flush_caches(&so_mod);
    log_info("so_flush_caches() passed.");

    so_initialize(&so_mod);
    log_info("so_initialize() passed.");

    gl_preload();
    log_info("gl_preload() passed.");

    jni_init();
    log_info("jni_init() passed.");
}
