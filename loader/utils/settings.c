/*
 * utils/settings.c
 *
 * Loader settings that can be set via the companion app.
 *
 * Copyright (C) 2021 TheFloW
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include <stdio.h>
#include <string.h>
#include "settings.h"

#define CONFIG_FILE_PATH DATA_PATH"config.txt"

float setting_leftStickDeadZone;
float setting_rightStickDeadZone;
int setting_fpsLock;
int setting_gfxDetail;
int setting_geometryDetail;
bool setting_enableMipMaps;
float setting_viewDistance;

void settings_reset() {
    setting_leftStickDeadZone = 0.11f;
    setting_rightStickDeadZone = 0.11f;
    setting_fpsLock = 0;
    setting_gfxDetail = 1;
    setting_geometryDetail = 2;
    setting_enableMipMaps = false;
    setting_viewDistance = 1.10f;
}

void settings_load() {
    settings_reset();

    char buffer[30];
    int value;

    FILE *config = fopen(CONFIG_FILE_PATH, "r");

    if (config) {
        while (EOF != fscanf(config, "%[^ ] %d\n", buffer, &value)) {
            if (strcmp("leftStickDeadZone", buffer) == 0) setting_leftStickDeadZone = ((float)value / 100.f);
            else if (strcmp("rightStickDeadZone", buffer) == 0) setting_rightStickDeadZone = ((float)value / 100.f);
            else if (strcmp("fpsLock", buffer) == 0) setting_fpsLock = value;
            else if (strcmp("gfxDetail", buffer) == 0) setting_gfxDetail = (int)value;
            else if (strcmp("geometryDetail", buffer) == 0) setting_geometryDetail = (int)value;
            else if (strcmp("enableMipMaps", buffer) == 0) setting_enableMipMaps = (bool)value;
            else if (strcmp("viewDistance", buffer) == 0) setting_viewDistance = ((float)value / 100.f);
        }
        fclose(config);
    }
}

void settings_save() {
    FILE *config = fopen(CONFIG_FILE_PATH, "w+");

    if (config) {
        fprintf(config, "%s %d\n", "leftStickDeadZone", (int)(setting_leftStickDeadZone * 100.f));
        fprintf(config, "%s %d\n", "rightStickDeadZone", (int)(setting_rightStickDeadZone * 100.f));
        fprintf(config, "%s %d\n", "fpsLock", (int)setting_fpsLock);
        fprintf(config, "%s %d\n", "gfxDetail", (int)setting_gfxDetail);
        fprintf(config, "%s %d\n", "geometryDetail", (int)setting_geometryDetail);
        fprintf(config, "%s %d\n", "enableMipMaps", (int)setting_enableMipMaps);
        fprintf(config, "%s %d\n", "viewDistance", (int)(setting_viewDistance * 100.f));
        fclose(config);
    }
}
