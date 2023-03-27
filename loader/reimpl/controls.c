#include <math.h>
#include <stdint.h>
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/kernel/clib.h>
#include <so_util/so_util.h>
#include <FalsoJNI/FalsoJNI.h>
#include "utils/logger.h"
#include "utils/settings.h"

#define SCREEN_W 960
#define SCREEN_H 544

#define AKEYCODE_DPAD_UP 19
#define AKEYCODE_DPAD_DOWN 20
#define AKEYCODE_DPAD_LEFT 21
#define AKEYCODE_DPAD_RIGHT 22

#define AKEYCODE_BUTTON_X 99
#define AKEYCODE_BUTTON_Y 100

#define AKEYCODE_DPAD_CENTER 23
#define AKEYCODE_BACK 4

#define AKEYCODE_BUTTON_L1 102
#define AKEYCODE_BUTTON_R1 103

#define AKEYCODE_BUTTON_SELECT 109
#define AKEYCODE_BUTTON_START 108

#define AKEYCODE_MENU 82
#define AKEYCODE_X 52

typedef struct {
    uint32_t sce_button;
    uint32_t android_button;
} ButtonMapping;

typedef struct {
    uint32_t sce_button;
    uint32_t x;
    uint32_t y;
} ButtonToTouchMapping;

static ButtonMapping mapping[] = {
        { SCE_CTRL_START,     AKEYCODE_BUTTON_START },
};

static ButtonToTouchMapping mapping_touch[] = {
        { SCE_CTRL_L1,        694, 489 }, // L — Aim // should become x 879 y 531 if we are aiming already
        { SCE_CTRL_R1,        694, 489 }, // R — Shoot
        { SCE_CTRL_TRIANGLE,  828, 356 }, // Triangle — Use cannon
        { SCE_CTRL_SQUARE,    865, 445 }, // Square — Sword
        { SCE_CTRL_CROSS,     792, 391 }, // X — Jump
        { SCE_CTRL_SELECT,    70,  86  }, // Select — Minimap
        { SCE_CTRL_CIRCLE,    913, 355 }, // O — Parry / special attack

        { -1,                 870, 317 }, // R+Square combo
};

float touchLx_radius = 78;
float touchLy_radius = 78;
float touchRx_radius = 42;
float touchRy_radius = 42;

float touchLx_base = 144;
float touchLy_base = 414;
float touchRx_base = 662;
float touchRy_base = 237;

int (* nativeOnTouch)(void *env, void *obj, int action, int x, int y, int index);

extern void (*CheckInputKey)(int keycode, int action,int repeats,int scancode);
extern int (*AbsorbKey)(int keycode);
extern void * (*Application__GetInstance)();

extern int * isPressKey;
extern int isInAimMode();
extern int isOnCannon();
extern int canCallHorse();
extern int callHorse();

extern so_module so_mod;

int lastX[2] = { -1, -1 };
int lastY[2] = { -1, -1 };

uint32_t old_buttons = 0, current_buttons = 0, pressed_buttons = 0, released_buttons = 0;

float lastLx = 0.0f, lastLy = 0.0f, lastRx = 0.0f, lastRy = 0.0f;
float lx = 0.0f, ly = 0.0f, rx = 0.0f, ry = 0.0f;

int lActive = 0, rActive = 0, lastLActive = 0, lastRActive = 0;
int fingerIdL = 11, fingerIdR = 12;

int rDown = 0;

float touchRx_last, touchRy_last, touchLx_last, touchLy_last;

float lerp(float x1, float y1, float x3, float y3, float x2) {
    return ((x2-x1)*(y3-y1) / (x3-x1)) + y1;
}

void coord_normalize(float *x, float *y, float deadZone, float maximum) {
    //radial and scaled deadzone
    //http://www.third-helix.com/2013/04/12/doing-thumbstick-dead-zones-right.html
    float analogX = (float) *x;
    float analogY = (float) *y;
    float magnitude = sqrt(analogX * analogX + analogY * analogY);
    if (magnitude >= deadZone) {
        float scalingFactor = maximum / magnitude * (magnitude - deadZone) / (maximum - deadZone);
        *x = (analogX * scalingFactor);
        *y = (analogY * scalingFactor);
    } else {
        *x = 0;
        *y = 0;
    }
}

void controls_init() {
    // Enable analog stick and touchscreen
    sceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, 1);

    nativeOnTouch = (void *)so_symbol(&so_mod, "Java_com_gameloft_android_ANMP_GloftSDHM_GameGLSurfaceView_nativeOnTouch");
}

void controls_poll() {
    SceTouchData touch;
    sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);

    for (int i = 0; i < 2; i++) {
        if (i < touch.reportNum) {
            int x = (int)((float)touch.report[i].x * (float)SCREEN_W / 1920.0f);
            int y = (int)((float)touch.report[i].y * (float)SCREEN_H / 1088.0f);

            if (lastX[i] == -1 || lastY[i] == -1) {
                sceClibPrintf("touchdown x %i y %i \n", x, y);
                nativeOnTouch(&jni, NULL, 1, x, y, i);
            }
            else if (lastX[i] != x || lastY[i] != y)
                nativeOnTouch(&jni, NULL, 2, x, y, i);
            lastX[i] = x;
            lastY[i] = y;
        } else {
            if (lastX[i] != -1 || lastY[i] != -1)
                nativeOnTouch(&jni, NULL, 0, lastX[i], lastY[i], i);
            lastX[i] = -1;
            lastY[i] = -1;
        }
    }

    int onCannon = isOnCannon();
    int canCallRoach = canCallHorse();

    SceCtrlData pad;
    sceCtrlPeekBufferPositiveExt2(0, &pad, 1);

    { // Gamepad buttons
        old_buttons = current_buttons;
        current_buttons = pad.buttons;
        pressed_buttons = current_buttons & ~old_buttons;
        released_buttons = ~current_buttons & old_buttons;

        for (int i = 0; i < sizeof(mapping) / sizeof(ButtonMapping); i++) {
            void * instance = Application__GetInstance();
            if (instance == 0) {
                log_info("instance is zero, skip");
                break;
            }

            if (pressed_buttons & mapping[i].sce_button) {

                if (AbsorbKey(mapping[i].android_button) == 0) {
                    logv_info("absorbed key 0x%x", mapping[i].android_button);
                    continue;
                }

                logv_info("sending down key 0x%x", mapping[i].android_button);
                CheckInputKey(mapping[i].android_button, 1, 0, mapping[i].android_button);

                *isPressKey = mapping[i].android_button;
            }
            if (released_buttons & mapping[i].sce_button) {
                if (AbsorbKey(mapping[i].android_button) == 0) {
                    logv_info("absorbed key 0x%x", mapping[i].android_button);
                    continue;
                }

                logv_info("sending up key 0x%x", mapping[i].android_button);
                CheckInputKey(mapping[i].android_button, 0, 0, mapping[i].android_button);
            }
        }

        // L — Aim // changes touch location depending on current state
        if (isInAimMode()) {
            mapping_touch[0].x = 879;
            mapping_touch[0].y = 531;
        } else {
            mapping_touch[0].x = 694;
            mapping_touch[0].y = 489;
        }

        // R — Shoot // changes touch location depending on current state
        if (onCannon) {
            mapping_touch[1].x = 806;
            mapping_touch[1].y = 484;
            mapping_touch[2].x = 694; // triangle functions same as L1 here
            mapping_touch[2].y = 489; // triangle functions same as L1 here
        } else {
            mapping_touch[1].x = 694;
            mapping_touch[1].y = 489;
            mapping_touch[2].x = 828;
            mapping_touch[2].y = 356;
        }

        static int combo1_active = 0;
        static int r1_pressed = 0;
        static int square_pressed = 0;

        static int press_count = 0;

        if (pressed_buttons & SCE_CTRL_R1) r1_pressed = 1;
        if (released_buttons & SCE_CTRL_R1) r1_pressed = 0;
        if (pressed_buttons & SCE_CTRL_SQUARE) square_pressed = 1;
        if (released_buttons & SCE_CTRL_SQUARE) square_pressed = 0;

        if (released_buttons & SCE_CTRL_TRIANGLE) press_count = 0;
        if (pressed_buttons & SCE_CTRL_TRIANGLE) press_count++;
        if (press_count > 0) press_count++;

        if (r1_pressed == 1 && square_pressed == 1 && combo1_active == 0) {
            combo1_active = 1;
            nativeOnTouch(&jni, NULL, 1, mapping_touch[7].x, mapping_touch[7].y, 20 + mapping_touch[7].sce_button);
        }

        if (canCallRoach && press_count > 15) {
            callHorse();
            press_count = 0;
        }

        for (int i = 0; i < sizeof(mapping_touch) / sizeof(ButtonToTouchMapping); i++) {
            void * instance = Application__GetInstance();
            if (instance == 0) {
                log_info("instance is zero, skip");
                break;
            }

            if (pressed_buttons & mapping_touch[i].sce_button) {
                if (!(combo1_active && (mapping_touch[i].sce_button == SCE_CTRL_SQUARE || mapping_touch[i].sce_button == SCE_CTRL_R1))) {
                    nativeOnTouch(&jni, NULL, 1, mapping_touch[i].x, mapping_touch[i].y, 20 + mapping_touch[i].sce_button);
                }
            }

            if (released_buttons & mapping_touch[i].sce_button) {
                if (!(combo1_active && (mapping_touch[i].sce_button == SCE_CTRL_SQUARE || mapping_touch[i].sce_button == SCE_CTRL_R1))) {
                    nativeOnTouch(&jni, NULL, 0, mapping_touch[i].x, mapping_touch[i].y, 20 + mapping_touch[i].sce_button);
                }
            }
        }

        if (pressed_buttons & SCE_CTRL_UP) {
            nativeOnTouch(&jni, NULL, 1, touchLx_base, touchLy_base, 20 + SCE_CTRL_UP);
            nativeOnTouch(&jni, NULL, 2, touchLx_base, touchLy_base - touchLy_radius, 20 + SCE_CTRL_UP);
        }

        if (pressed_buttons & SCE_CTRL_DOWN) {
            nativeOnTouch(&jni, NULL, 1, touchLx_base, touchLy_base, 20 + SCE_CTRL_DOWN);
            nativeOnTouch(&jni, NULL, 2, touchLx_base, touchLy_base + touchLy_radius, 20 + SCE_CTRL_DOWN);
        }

        if (pressed_buttons & SCE_CTRL_LEFT) {
            nativeOnTouch(&jni, NULL, 1, touchLx_base, touchLy_base, 20 + SCE_CTRL_LEFT);
            nativeOnTouch(&jni, NULL, 2, touchLx_base - touchLx_radius, touchLy_base, 20 + SCE_CTRL_LEFT);
        }

        if (pressed_buttons & SCE_CTRL_RIGHT) {
            nativeOnTouch(&jni, NULL, 1, touchLx_base, touchLy_base, 20 + SCE_CTRL_RIGHT);
            nativeOnTouch(&jni, NULL, 2, touchLx_base + touchLx_radius, touchLy_base, 20 + SCE_CTRL_RIGHT);
        }

        if (released_buttons & SCE_CTRL_UP) {
            nativeOnTouch(&jni, NULL, 0, touchLx_base, touchLy_base, 20 + SCE_CTRL_UP);
        }

        if (released_buttons & SCE_CTRL_DOWN) {
            nativeOnTouch(&jni, NULL, 0, touchLx_base, touchLy_base, 20 + SCE_CTRL_DOWN);
        }

        if (released_buttons & SCE_CTRL_LEFT) {
            nativeOnTouch(&jni, NULL, 0, touchLx_base, touchLy_base, 20 + SCE_CTRL_LEFT);
        }

        if (released_buttons & SCE_CTRL_RIGHT) {
            nativeOnTouch(&jni, NULL, 0, touchLx_base, touchLy_base, 20 + SCE_CTRL_RIGHT);
        }

        if (combo1_active == 1 && (square_pressed == 0 || r1_pressed == 0)) {
            combo1_active = 0;
            nativeOnTouch(&jni, NULL, 0, mapping_touch[7].x, mapping_touch[7].y, 20 + mapping_touch[7].sce_button);
        }
    }

    // Analog sticks

    if (rDown == 1) {
        nativeOnTouch(&jni, NULL, 0, touchRx_last, touchRy_last, fingerIdR+1);
        rDown = 0;
    }

    lx = ((float)pad.lx - 128.0f) / 128.0f;
    ly = ((float)pad.ly - 128.0f) / 128.0f;
    rx = ((float)pad.rx - 128.0f) / 256.0f; // We scale it down by 0.5 to reduce sensitivity
    ry = ((float)pad.ry - 128.0f) / 256.0f; // We scale it down by 0.5 to reduce sensitivity
    coord_normalize(&lx, &ly, setting_leftStickDeadZone, 0.8f);
    coord_normalize(&rx, &ry, setting_rightStickDeadZone, 0.9f);

    float touchLx = touchLx_base + (touchLx_radius * lx);
    float touchLy = touchLy_base + (touchLy_radius * (ly));
    float touchRx = touchRx_base + (touchRx_radius * rx);
    float touchRy = touchRy_base + (touchRy_radius * (ry));

    if (fabsf(rx) > 0.f || fabsf(ry) > 0.f) {
        rActive = 1;
    } else {
        rActive = 0;
    }

    if ((fabsf(lx) > 0.f || fabsf(ly) > 0.f) && !onCannon) {
        lActive = 1;
    } else if (lActive) {
        nativeOnTouch(&jni, NULL, 0, touchLx_last, touchLy_last, fingerIdL+1);
        lActive = 0;
    }

    if ((fabsf(rx) > 0.f || fabsf(ry) > 0.f) && !rDown) {
        nativeOnTouch(&jni, NULL, 1, touchRx_base, touchRy_base, fingerIdR+1);
        nativeOnTouch(&jni, NULL, 2, touchRx, touchRy, fingerIdR+1);
        rDown = 1;
    }

    if ((fabsf(lx) > 0.f || fabsf(ly) > 0.f) && !onCannon) {
        if (!lastLActive) {
            nativeOnTouch(&jni, NULL, 1, touchLx_base, touchLy_base, fingerIdL+1);
        }

        nativeOnTouch(&jni, NULL, 2, touchLx, touchLy, fingerIdL+1);
    }

    lastLActive = lActive;
    lastRActive = rActive;

    lastLx = lx;
    lastLy = ly;
    lastRx = rx;
    lastRy = ry;

    touchRx_last = touchRx;
    touchRy_last = touchRy;
    touchLx_last = touchLx;
    touchLy_last = touchLy;
}