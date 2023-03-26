/*
 * main.c
 *
 * ARMv7 Shared Libraries loader. Backstab HD edition
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2021-2023 Rinnegatamante
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "utils/init.h"
#include "utils/glutil.h"
#include "reimpl/controls.h"
#include "utils/settings.h"

#include <psp2/kernel/threadmgr.h>

#include <FalsoJNI/FalsoJNI.h>
#include <so_util/so_util.h>
#include <psp2/kernel/processmgr.h>
#include <sched.h>

int _newlib_heap_size_user = 256 * 1024 * 1024;

so_module so_mod;

int main(int argc, char* argv[]) {
    soloader_init_all();

    int (* JNI_OnLoad)(void *jvm) = (void *)so_symbol(&so_mod, "JNI_OnLoad");

    int (* Java_com_gameloft_android_ANMP_GloftSDHM_GameRenderer_nativeResize)(void *env, void *obj, int width, int height) = (void *)so_symbol(&so_mod, "Java_com_gameloft_android_ANMP_GloftSDHM_GameRenderer_nativeResize");
    int (* Java_com_gameloft_android_ANMP_GloftSDHM_GameRenderer_nativeInit)(void *env, void *obj, int manufacturer, int width, int height, char *version) = (void *)so_symbol(&so_mod, "Java_com_gameloft_android_ANMP_GloftSDHM_GameRenderer_nativeInit");
    int (* Java_com_gameloft_android_ANMP_GloftSDHM_GameRenderer_nativeRender)() = (void *)so_symbol(&so_mod, "Java_com_gameloft_android_ANMP_GloftSDHM_GameRenderer_nativeRender");

    int (* Java_com_gameloft_android_ANMP_GloftSDHM_Game_nativeInit)() = (void *)so_symbol(&so_mod, "Java_com_gameloft_android_ANMP_GloftSDHM_Game_nativeInit");

    JNI_OnLoad(&jvm);

    gl_init();
    if (setting_fpsLock > 0 && setting_fpsLock <= 30)
        eglSwapInterval(0, 2);

    Java_com_gameloft_android_ANMP_GloftSDHM_GameRenderer_nativeInit(&jni, NULL, 0, 960, 544, "1.0");
    Java_com_gameloft_android_ANMP_GloftSDHM_Game_nativeInit();
    Java_com_gameloft_android_ANMP_GloftSDHM_GameRenderer_nativeResize(&jni, NULL, 960, 544);

    if (setting_fpsLock == 0 || setting_fpsLock == 30) {
        while (1) {
            controls_poll();
            Java_com_gameloft_android_ANMP_GloftSDHM_GameRenderer_nativeRender();
            vglSwapBuffers(0);
        }
    }

    if (setting_fpsLock == 25 || setting_fpsLock == 40) {
        uint32_t last_render_time = sceKernelGetProcessTimeLow();
        uint32_t delta = (1000000 / (setting_fpsLock+1));

        while (1) {
            controls_poll();
            Java_com_gameloft_android_ANMP_GloftSDHM_GameRenderer_nativeRender();

            while (sceKernelGetProcessTimeLow() - last_render_time < delta) {
                sched_yield();
            }

            last_render_time = sceKernelGetProcessTimeLow();

            vglSwapBuffers(0);
        }
    }

    sceKernelExitDeleteThread(0);
}
