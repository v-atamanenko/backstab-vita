/*
 * falojni_impl.c
 *
 * Implementations for JNI functions and fields to be used by FalsoJNI.
 *
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include <FalsoJNI/FalsoJNI_Impl.h>

#include "utils/logger.h"

#include "audioPlayer.c"

jfloat GetPhoneCPUFreq(jmethodID id, va_list args) {
    dbg;
    return 0;
}

jint getManufacture(jmethodID id, va_list args) {
    dbg;
    return 0;
}

jint GetOSVersion(jmethodID id, va_list args) {
    dbg;
    return 0;
}

jint GetPhoneLanguage(jmethodID id, va_list args) {
    dbg;
    return 0;
}

jint GetTextureFormat(jmethodID id, va_list args) {
    dbg;
    return 0;
}

jint isWifiEnabled(jmethodID id, va_list args) {
    dbg;
    return 0;
}

jlong GetCurrentTime(jmethodID id, va_list args) {
    dbg;
    return 0;
}


jobject GetPhoneCPUName(jmethodID id, va_list args) {
    dbg;
    return "nothing";
}

jobject GetPhoneGPUName(jmethodID id, va_list args) {
    dbg;
    return "nothing";
}

jobject GetPhoneManufacturer(jmethodID id, va_list args) {
    dbg;
    return "nothing";
}

jobject GetPhoneModel(jmethodID id, va_list args) {
    dbg;
    return "nothing";
}


void Exit(jmethodID id, va_list args) {
    dbg;
}

void GameTracking(jmethodID id, va_list args) {
    dbg;
}

void GC(jmethodID id, va_list args) {
    dbg;
}

void launchGLLive(jmethodID id, va_list args) {
    dbg;
}

void launchIGP(jmethodID id, va_list args) {
    dbg;
}

void notifyTrophy(jmethodID id, va_list args) {
    dbg;
}

void openBrowser(jmethodID id, va_list args) {
    dbg;
}

void Pause(jmethodID id, va_list args) {
    dbg;
}

void PrintDebug(jmethodID id, va_list args) {
    const char * str = va_arg(args, char*);
    logv_info("%s", str);
}

void sendAppToBackground(jmethodID id, va_list args) {
    dbg;
}


NameToMethodID nameToMethodId[] = {
    { 1, "Exit", METHOD_TYPE_VOID }, // ()V
    { 2, "openBrowser", METHOD_TYPE_VOID }, // (Ljava/lang/String;)V
    { 3, "isWifiEnabled", METHOD_TYPE_INT }, // ()I
    { 4, "Pause", METHOD_TYPE_VOID }, // ()V
    { 5, "GetPhoneLanguage", METHOD_TYPE_INT }, // ()I
    { 6, "launchGLLive", METHOD_TYPE_VOID }, // (I)V
    { 7, "getManufacture", METHOD_TYPE_INT }, // ()I
    { 8, "notifyTrophy", METHOD_TYPE_VOID }, // (I)V
    { 9, "launchIGP", METHOD_TYPE_VOID }, // (I)V
    { 10, "GetCurrentTime", METHOD_TYPE_LONG }, // ()J
    { 11, "GetTextureFormat", METHOD_TYPE_INT }, // ()I
    { 12, "PrintDebug", METHOD_TYPE_VOID }, // (Ljava/lang/String;)V
    { 13, "GetPhoneManufacturer", METHOD_TYPE_OBJECT }, // ()Ljava/lang/String;
    { 14, "GetPhoneModel", METHOD_TYPE_OBJECT }, // ()Ljava/lang/String;
    { 15, "GetPhoneCPUName", METHOD_TYPE_OBJECT }, // ()Ljava/lang/String;
    { 16, "GetPhoneCPUFreq", METHOD_TYPE_FLOAT }, // ()F
    { 17, "GetPhoneGPUName", METHOD_TYPE_OBJECT }, // ()Ljava/lang/String;
    { 18, "GC", METHOD_TYPE_VOID }, // (I)V
    { 19, "GetOSVersion", METHOD_TYPE_INT }, // ()I
    { 20, "GameTracking", METHOD_TYPE_VOID }, // ()V
    { 21, "sendAppToBackground", METHOD_TYPE_VOID }, // ()V
    { 22, "android/media/AudioTrack/<init>", METHOD_TYPE_OBJECT }, // (IIIIII)V
    { 23, "getMinBufferSize", METHOD_TYPE_INT }, // (III)I
    { 24, "play", METHOD_TYPE_VOID }, // ()V
    { 25, "pause", METHOD_TYPE_VOID }, // ()V
    { 26, "stop", METHOD_TYPE_VOID }, // ()V
    { 27, "release", METHOD_TYPE_VOID }, // ()V
    { 28, "write", METHOD_TYPE_INT } // ([BII)I
};

MethodsBoolean methodsBoolean[] = {};
MethodsByte methodsByte[] = {};
MethodsChar methodsChar[] = {};
MethodsDouble methodsDouble[] = {};
MethodsFloat methodsFloat[] = {
    { 16, GetPhoneCPUFreq }
};
MethodsInt methodsInt[] = {
    { 3, isWifiEnabled },
    { 5, GetPhoneLanguage },
    { 7, getManufacture },
    { 11, GetTextureFormat },
    { 19, GetOSVersion },
    { 23, audioTrack_getMinBufferSize },
    { 28, audioTrack_write }
};
MethodsLong methodsLong[] = {
    { 10, GetCurrentTime }
};
MethodsObject methodsObject[] = {
    { 13, GetPhoneManufacturer },
    { 14, GetPhoneModel },
    { 15, GetPhoneCPUName },
    { 17, GetPhoneGPUName },
    { 22, audioTrack_init }
};
MethodsShort methodsShort[] = {};
MethodsVoid methodsVoid[] = {
    
    { 1, Exit },
    { 2, openBrowser },
    { 4, Pause },
    { 6, launchGLLive },
    { 8, notifyTrophy },
    { 9, launchIGP },
    { 20, GameTracking },
    { 18, GC },
    { 12, PrintDebug },
    { 21, sendAppToBackground },
    { 24, audioTrack_play },
    { 25, audioTrack_pause },
    { 26, audioTrack_stop },
    { 27, audioTrack_release },
};

NameToFieldID nameToFieldId[] = {};

FieldsBoolean fieldsBoolean[] = {};
FieldsByte fieldsByte[] = {};
FieldsChar fieldsChar[] = {};
FieldsDouble fieldsDouble[] = {};
FieldsFloat fieldsFloat[] = {};
FieldsInt fieldsInt[] = {};
FieldsObject fieldsObject[] = {};
FieldsLong fieldsLong[] = {};
FieldsShort fieldsShort[] = {};

__FALSOJNI_IMPL_CONTAINER_SIZES
