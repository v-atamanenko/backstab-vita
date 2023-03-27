#include <kubridge.h>
#include <so_util/so_util.h>

#include "utils/settings.h"

void (*CheckInputKey)(int keycode, int action,int repeats,int scancode);
void (*AbsorbKey)(int keycode);
void * (*Application__GetInstance)();

int * isPressKey;

void * (*CLevel__GetLevel)();
void * (*CLevel__GetPlayerComponent)(void * this);
int (*PlayerComponent_IsInAimMode)(void * this);
int (*PlayerComponent_IsOnCannon)(void * this);
int (*PlayerComponent_IsOnHorse)(void * this);
int (*PlayerComponent_CanCallHorse)(void * this);
int (*PlayerComponent_CallHorse)(void * this);
int (*PlayerComponent_CanUseVengeance)(void * this);
int (*PlayerComponent_CanUseInteractButton)(void * this);
void (*PlayerComponent_IncrementGrenadeSelection)(void * this, int p);
void (*PlayerComponent_EnterAimMode)(void * this);

void nextGrenade() {
    void * lvl = CLevel__GetLevel();
    if (lvl) {
        void * pc = CLevel__GetPlayerComponent(lvl);
        if (pc) {
            PlayerComponent_IncrementGrenadeSelection(pc, 1);
            PlayerComponent_EnterAimMode(pc);
        }
    }
}

int isInAimMode() {
    void * lvl = CLevel__GetLevel();
    if (lvl) {
        void * pc = CLevel__GetPlayerComponent(lvl);
        if (pc) {
            return PlayerComponent_IsInAimMode(pc);
        }
    }
    return 0;
}

int isOnCannon() {
    void * lvl = CLevel__GetLevel();
    if (lvl) {
        void * pc = CLevel__GetPlayerComponent(lvl);
        if (pc) {
            return PlayerComponent_IsOnCannon(pc);
        }
    }
    return 0;
}

int canCallHorse() {
    void * lvl = CLevel__GetLevel();
    if (lvl) {
        void * pc = CLevel__GetPlayerComponent(lvl);
        if (pc) {
            return PlayerComponent_CanCallHorse(pc);
        }
    }
    return 0;
}

int callHorse() {
    void * lvl = CLevel__GetLevel();
    if (lvl) {
        void * pc = CLevel__GetPlayerComponent(lvl);
        if (pc) {
            return PlayerComponent_CallHorse(pc);
        }
    }
    return 0;
}

int isOnHorse() {
    void * lvl = CLevel__GetLevel();
    if (lvl) {
        void * pc = CLevel__GetPlayerComponent(lvl);
        if (pc) {
            return PlayerComponent_IsOnHorse(pc);
        }
    }
    return 0;
}

int canUseVengeance() {
    void * lvl = CLevel__GetLevel();
    if (lvl) {
        void * pc = CLevel__GetPlayerComponent(lvl);
        if (pc) {
            if (PlayerComponent_CanUseVengeance(pc)) {
                if (!PlayerComponent_CanUseInteractButton(pc)) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

so_hook loadControlScheme_hook;

void LoadControlScheme(int x) {
    SO_CONTINUE(void *, loadControlScheme_hook, x);
    int8_t * dpad_open = (int8_t *) so_symbol(&so_mod, "dpad_open");
    *dpad_open = 1;
}

void patch__controls() {
    CLevel__GetLevel = (uintptr_t)so_symbol(&so_mod, "_ZN6CLevel8GetLevelEv");
    CLevel__GetPlayerComponent = (uintptr_t)so_symbol(&so_mod, "_ZN6CLevel18GetPlayerComponentEv");
    PlayerComponent_IsInAimMode = (uintptr_t)so_symbol(&so_mod, "_ZN15PlayerComponent11IsInAimModeEv");
    PlayerComponent_IsOnCannon = (uintptr_t)so_symbol(&so_mod, "_ZN15PlayerComponent10IsOnCannonEv");
    PlayerComponent_IsOnHorse = (uintptr_t)so_symbol(&so_mod, "_ZN15PlayerComponent9IsOnHorseEv");
    PlayerComponent_CanCallHorse = (uintptr_t)so_symbol(&so_mod, "_ZN15PlayerComponent12CanCallHorseEv");
    PlayerComponent_CallHorse = (uintptr_t)so_symbol(&so_mod, "_ZN15PlayerComponent9CallHorseEv");
    PlayerComponent_CanUseVengeance = (uintptr_t)so_symbol(&so_mod, "_ZN15PlayerComponent15CanUseVengeanceEv");
    PlayerComponent_CanUseInteractButton = (uintptr_t)so_symbol(&so_mod, "_ZN15PlayerComponent20CanUseInteractButtonEv");
    PlayerComponent_IncrementGrenadeSelection = (uintptr_t)so_symbol(&so_mod, "_ZN15PlayerComponent25IncrementGrenadeSelectionEi");
    PlayerComponent_EnterAimMode = (uintptr_t)so_symbol(&so_mod, "_ZN15PlayerComponent12EnterAimModeEv");

    CheckInputKey = (uintptr_t)so_symbol(&so_mod, "_Z13CheckInputKeyi17EVENT_ACTION_TYPEii");
    AbsorbKey = (uintptr_t)so_symbol(&so_mod, "_Z9AbsorbKeyi");
    Application__GetInstance = (uintptr_t)so_symbol(&so_mod, "_ZN11Application11GetInstanceEv");
    isPressKey = (uintptr_t)so_symbol(&so_mod, "isPressKey");

    loadControlScheme_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_Z17LoadControlSchemei"), (uintptr_t)&LoadControlScheme);
}
