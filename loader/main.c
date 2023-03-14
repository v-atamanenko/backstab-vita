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
#include "utils/logger.h"
#include "utils/settings.h"

#include <psp2/kernel/threadmgr.h>

#include <FalsoJNI/FalsoJNI.h>
#include <so_util/so_util.h>

int _newlib_heap_size_user = 220 * 1024 * 1024;

so_module so_mod;

#define SCREEN_W 960
#define SCREEN_H 544

#include <psp2/touch.h> 

int main(int argc, char* argv[]) {
    soloader_init_all();

    int (* Java_com_gameloft_android_ANMP_GloftSDHM_GameRenderer_nativeResize)(void *env, void *obj, int width, int height) = (void *)so_symbol(&so_mod, "Java_com_gameloft_android_ANMP_GloftSDHM_GameRenderer_nativeResize");
    int (* Java_com_gameloft_android_ANMP_GloftSDHM_GameRenderer_nativeInit)(void *env, void *obj, int manufacturer, int width, int height, char *version) = (void *)so_symbol(&so_mod, "Java_com_gameloft_android_ANMP_GloftSDHM_GameRenderer_nativeInit");
    int (* Java_com_gameloft_android_ANMP_GloftSDHM_GameRenderer_nativeRender)() = (void *)so_symbol(&so_mod, "Java_com_gameloft_android_ANMP_GloftSDHM_GameRenderer_nativeRender");

    int (* Java_com_gameloft_android_ANMP_GloftSDHM_Game_nativeInit)() = (void *)so_symbol(&so_mod, "Java_com_gameloft_android_ANMP_GloftSDHM_Game_nativeInit");
    
    int (* Java_com_gameloft_android_ANMP_GloftSDHM_GameGLSurfaceView_nativeOnTouch)(void *env, void *obj, int action, int x, int y, int index) = (void *)so_symbol(&so_mod, "Java_com_gameloft_android_ANMP_GloftSDHM_GameGLSurfaceView_nativeOnTouch");
    
    printf("nativeInit %x\n", Java_com_gameloft_android_ANMP_GloftSDHM_GameRenderer_nativeInit);
    Java_com_gameloft_android_ANMP_GloftSDHM_GameRenderer_nativeInit(&jni, NULL, 0, SCREEN_W, SCREEN_H, "1.0");
    printf("nativeResize\n");
    Java_com_gameloft_android_ANMP_GloftSDHM_GameRenderer_nativeResize(&jni, NULL, SCREEN_W, SCREEN_H);
    
    printf("nativeInit2\n");
    Java_com_gameloft_android_ANMP_GloftSDHM_Game_nativeInit();

    int lastX[2] = { -1, -1 };
    int lastY[2] = { -1, -1 };

    while (1) {
        SceTouchData touch;
        sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);

        for (int i = 0; i < 2; i++) {
            if (i < touch.reportNum) {
                int x = (int)((float)touch.report[i].x * (float)SCREEN_W / 1920.0f);
                int y = (int)((float)touch.report[i].y * (float)SCREEN_H / 1088.0f);

                if (lastX[i] == -1 || lastY[i] == -1)
                    Java_com_gameloft_android_ANMP_GloftSDHM_GameGLSurfaceView_nativeOnTouch(&jni, NULL, 1, x, y, i);
                else if (lastX[i] != x || lastY[i] != y)
                    Java_com_gameloft_android_ANMP_GloftSDHM_GameGLSurfaceView_nativeOnTouch(&jni, NULL, 2, x, y, i);
                lastX[i] = x;
                lastY[i] = y;
            } else {
                if (lastX[i] != -1 || lastY[i] != -1)
                    Java_com_gameloft_android_ANMP_GloftSDHM_GameGLSurfaceView_nativeOnTouch(&jni, NULL, 0, lastX[i], lastY[i], i);
                lastX[i] = -1;
                lastY[i] = -1;
            }
        }
    
        printf("nativeRender\n");
        Java_com_gameloft_android_ANMP_GloftSDHM_GameRenderer_nativeRender();
        vglSwapBuffers(0);
    }

    sceKernelExitDeleteThread(0);
}
