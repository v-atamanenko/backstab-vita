#include <kubridge.h>
#include <so_util/so_util.h>

#include "utils/settings.h"

so_hook GameConfig__CalculateDevicePower_hook;

void GameConfig__CalculateDevicePower(void * this) {
    SO_CONTINUE(void *, GameConfig__CalculateDevicePower_hook, this);

    int8_t * hasLowPerformanceGfx = (int8_t *)so_symbol(&so_mod, "_ZN13CGameSettings22s_hasLowPerformanceGfxE");
    int8_t * hasHighPerformanceGfx = (int8_t *)so_symbol(&so_mod, "_ZN13CGameSettings23s_hasHighPerformanceGfxE");

    int8_t * hasLowPerformanceGeom = (int8_t *)so_symbol(&so_mod, "_ZN13CGameSettings23s_hasLowPerformanceGeomE");
    int8_t * hasMedPerformanceGeom = (int8_t *)so_symbol(&so_mod, "_ZN13CGameSettings23s_hasMedPerformanceGeomE");
    int8_t * hasHighPerformanceGeom = (int8_t *)so_symbol(&so_mod, "_ZN13CGameSettings24s_hasHighPerformanceGeomE");

    *hasLowPerformanceGfx = (int8_t)(setting_gfxDetail == 0);
    *hasHighPerformanceGfx = (int8_t)(setting_gfxDetail == 1);

    *hasLowPerformanceGeom = (int8_t)(setting_geometryDetail == 0);
    *hasMedPerformanceGeom = (int8_t)(setting_geometryDetail == 1);
    *hasHighPerformanceGeom = (int8_t)(setting_geometryDetail == 2);
}

so_hook GameConfig__AutoConfig_hook;

void GameConfig__AutoConfig(void * this) {
    SO_CONTINUE(void *, GameConfig__AutoConfig_hook, this);

    float * viewDistanceFactor = (float *) so_symbol(&so_mod, "_ZN13CGameSettings20s_viewDistanceFactorE");
    * viewDistanceFactor = setting_viewDistance;
}

void patch__graphics() {
    GameConfig__CalculateDevicePower_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN11CGameConfig20CalculateDevicePowerEv"), (uintptr_t)&GameConfig__CalculateDevicePower);
    GameConfig__AutoConfig_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN11CGameConfig10AutoConfigEv"), (uintptr_t)&GameConfig__AutoConfig);
}
