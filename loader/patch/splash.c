#include <string.h>
#include <kubridge.h>
#include <so_util/so_util.h>

#include "utils/settings.h"

static volatile int8_t splash_hook_enabled;

void * (*CSpriteManager__GetSprite)(void *this,char *param_1);
void patch__splash_disable();

so_hook CSpriteManager__LoadSprite_hook;
so_hook _ZN11GS_MainMenuC1Ev_hook;

void * GS_MainMenu__Constructor(void * this) {
    sceClibPrintf("Splash patch disabled\n");
    patch__splash_disable();
    return SO_CONTINUE(void *, _ZN11GS_MainMenuC1Ev_hook, this);
}

int CSpriteManager__LoadSprite(void *this, char *param_1, char *param_2, int8_t param_3) {
    if (strstr(param_2, "loading_menu.tga")) {
        return SO_CONTINUE(int, CSpriteManager__LoadSprite_hook, this, "loading_menu_fixed.bsprite", "loading_menu_fixed.tga", param_3);
    }

    if (strstr(param_2, "splash.tga")) {
        int ret = SO_CONTINUE(int, CSpriteManager__LoadSprite_hook, this, "splash.bsprite", "splash_fixed.tga", param_3);

        void ** _ZN14CSpriteManager9SingletonE = (uintptr_t)so_symbol(&so_mod, "_ZN14CSpriteManager9SingletonE");
        void * spr = CSpriteManager__GetSprite(*_ZN14CSpriteManager9SingletonE, "splash.bsprite");

        uint16_t ** sizes = (spr + 20);
        sizes[0][4] = 768;
        sizes[0][10] = 1280;
        sizes[0][20] = 1280;
        sizes[0][26] = 768;

        return ret;
    }

    return SO_CONTINUE(int, CSpriteManager__LoadSprite_hook, this, param_1, param_2, param_3);
}

void patch__splash() {
    splash_hook_enabled = 1;
    CSpriteManager__GetSprite = (uintptr_t)so_symbol(&so_mod, "_ZN14CSpriteManager9GetSpriteEPKc");
    CSpriteManager__LoadSprite_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN14CSpriteManager10LoadSpriteEPKcS1_b"), (uintptr_t)&CSpriteManager__LoadSprite);

    _ZN11GS_MainMenuC1Ev_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN11GS_MainMenuC1Ev"), (uintptr_t)&GS_MainMenu__Constructor);
}

void patch__splash_disable() {
    if (splash_hook_enabled == 1) {
        splash_hook_enabled = 0;
        so_unhook(&CSpriteManager__LoadSprite_hook);
    }
}
