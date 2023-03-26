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

#include "reimpl/controls.h"

#include "dynlib.h"
#include "patch.h"

#include <string.h>

#include <psp2/appmgr.h>
#include <psp2/apputil.h>
#include <psp2/kernel/clib.h>
#include <psp2/power.h>

#include <FalsoJNI/FalsoJNI.h>
#include <so_util/so_util.h>
#include <fios/fios.h>

// Base address for the Android .so to be loaded at
#define LOAD_ADDRESS 0x98000000

extern so_module so_mod;

void soloader_init_all() {
    // Check if we want to start the configurator app
    sceAppUtilInit(&(SceAppUtilInitParam){}, &(SceAppUtilBootParam){});
    SceAppUtilAppEventParam eventParam;
    sceClibMemset(&eventParam, 0, sizeof(SceAppUtilAppEventParam));
    sceAppUtilReceiveAppEvent(&eventParam);
    if (eventParam.type == 0x05) {
        char buffer[2048];
        sceAppUtilAppEventParseLiveArea(&eventParam, buffer);
        if (strstr(buffer, "-config"))
            sceAppMgrLoadExec("app0:/configurator.bin", NULL, NULL);
    }

    // Set default overclock values
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

    fios_init();
    log_info("fios init passed.");

    if (!module_loaded("kubridge"))
        fatal_error("Error: kubridge.skprx is not installed.");
    log_info("kubridge check passed.");

    if (!file_exists(SO_PATH)) {
        fatal_error("Looks like you haven't installed the data files for this "
                    "port, or they are in an incorrect location. Please make "
                    "sure that you have %s file exactly at that path.", SO_PATH);
    }

    if (!file_exists(DATA_PATH"com.gameloft.android.ANMP.GloftSDHM/files/splash_fixed.pvr")) {
        cp("app0:data/splash_fixed.pvr", DATA_PATH"com.gameloft.android.ANMP.GloftSDHM/files/splash_fixed.pvr");
        cp("app0:data/loading_menu_fixed.pvr", DATA_PATH"com.gameloft.android.ANMP.GloftSDHM/files/loading_menu_fixed.pvr");
        cp("app0:data/loading_menu_fixed.bsprite", DATA_PATH"com.gameloft.android.ANMP.GloftSDHM/files/loading_menu_fixed.bsprite");
        cp("app0:data/control0", DATA_PATH"com.gameloft.android.ANMP.GloftSDHM/files/control0");
    }

    if (so_file_load(&so_mod, SO_PATH, LOAD_ADDRESS) < 0)
        fatal_error("Error: could not load %s.", SO_PATH);

    settings_load();
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

    controls_init();
    log_info("controls_init() passed.");
}
