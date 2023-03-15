/*
 * patch.c
 *
 * Patching some of the .so internal functions or bridging them to native for
 * better compatibility.
 *
 * Copyright (C) 2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "patch.h"

#include <kubridge.h>
#include <so_util/so_util.h>

#include <psp2/kernel/clib.h>

extern so_module so_mod;

int S_SetScreenSize() {
    sceClibPrintf("called S_SetScreenSize\n");
}

int _ZN6glitch16CAndroidOSDevice12createKeyMapEv() {
    sceClibPrintf("called _ZN6glitch16CAndroidOSDevice12createKeyMapEv\n");
    return 0;
}

static char buffer_log[2048];
static char buffer_log_2[2048];

int osPrinterPrint(const char * fmt, ...) {
    sceClibSnprintf(buffer_log, sizeof(buffer_log), "[GAME:] %s\n", fmt);

    va_list list;
    va_start(list, fmt);
    sceClibVsnprintf(buffer_log_2, sizeof(buffer_log_2), buffer_log, list);
    va_end(list);

    sceClibPrintf(buffer_log_2);
}

int osPrinterLog(const char * fmt, int i) {
    sceClibPrintf("[GAME:%i] %s\n", i, fmt);
}

int ** lockPointer1;
int ** lockPointer2;
int ** lockPointer3;
int ** lockPointer4;

void so_patch(void) {
        hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN6glitch2os7Printer5printEPKcz"), (uintptr_t)&osPrinterPrint);
        hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN6glitch2os7Printer3logEPKcNS_10ELOG_LEVELE"), (uintptr_t)&osPrinterLog);
        hook_addr((uintptr_t)so_symbol(&so_mod, "S_SetScreenSize"), (uintptr_t)&S_SetScreenSize);
        hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN6glitch16CAndroidOSDevice12createKeyMapEv"), (uintptr_t)&_ZN6glitch16CAndroidOSDevice12createKeyMapEv);

        lockPointer1 = (uintptr_t)so_symbol(&so_mod, "lockPointer1");
        lockPointer2 = (uintptr_t)so_symbol(&so_mod, "lockPointer2");
        lockPointer3 = (uintptr_t)so_symbol(&so_mod, "lockPointer3");
        lockPointer4 = (uintptr_t)so_symbol(&so_mod, "lockPointer4");

        *lockPointer1 = malloc(4);
        **lockPointer1 = 1;

        *lockPointer2 = malloc(4);
        **lockPointer2 = 1;

        *lockPointer3 = malloc(4);
        **lockPointer3 = 1;

        *lockPointer4 = malloc(4);
        **lockPointer4 = 1;


}
