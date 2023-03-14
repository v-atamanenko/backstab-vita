/*
 * dynlib.c
 *
 * Resolving dynamic imports of the .so.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2021 Rinnegatamante
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

// Disable IDE complaints about _identifiers and global interfaces
#pragma ide diagnostic ignored "bugprone-reserved-identifier"
#pragma ide diagnostic ignored "cppcoreguidelines-interfaces-global-init"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

#include "dynlib.h"

#include <OpenSLES.h>
#include <math.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/processmgr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/unistd.h>

#include <so_util/so_util.h>

#include "utils/utils.h"
#include "utils/glutil.h"
#include "utils/settings.h"
#include "reimpl/io.h"
#include "reimpl/log.h"
#include "reimpl/mem.h"
#include "reimpl/pthr.h"

extern void *__aeabi_atexit;
extern void *__cxa_pure_virtual;
extern void *__stack_chk_fail;
extern void *__stack_chk_guard;

extern void * __aeabi_d2f;
extern void * __aeabi_d2iz;
extern void * __aeabi_dadd;
extern void * __aeabi_dcmpeq;
extern void * __aeabi_dcmpgt;
extern void * __aeabi_dcmplt;
extern void * __aeabi_ddiv;
extern void * __aeabi_dmul;
extern void * __aeabi_dsub;
extern void * __aeabi_f2d;
extern void * __aeabi_f2iz;
extern void * __aeabi_f2lz;
extern void * __aeabi_f2ulz;
extern void * __aeabi_fadd;
extern void * __aeabi_fcmpeq;
extern void * __aeabi_fcmpge;
extern void * __aeabi_fcmpgt;
extern void * __aeabi_fcmple;
extern void * __aeabi_fcmplt;
extern void * __aeabi_fdiv;
extern void * __aeabi_fmul;
extern void * __aeabi_fsub;
extern void * __aeabi_i2d;
extern void * __aeabi_i2f;
extern void * __aeabi_idiv;
extern void * __aeabi_idivmod;
extern void * __aeabi_l2f;
extern void * __aeabi_ldivmod;
extern void * __aeabi_lmul;
extern void * __aeabi_ui2d;
extern void * __aeabi_ui2f;
extern void * __aeabi_uidiv;
extern void * __aeabi_uidivmod;
extern void * __aeabi_ul2f;
extern void * __aeabi_uldivmod;
extern void * __dso_handle;
extern void * __swbuf;
extern void * _ZdaPv;
extern void * _ZdlPv;
extern void * _Znaj;
extern void * _Znwj;

extern const char *BIONIC_ctype_;
extern const short *BIONIC_tolower_tab_;
extern const short *BIONIC_toupper_tab_;

static FILE __sF_fake[3];

void glDrawElementsHook(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {
    if (mode != GL_POINTS)
        glDrawElements(mode, count, type, indices);
}

void glDrawArraysHook(GLenum mode, GLint first, GLsizei count) {
    if (mode != GL_POINTS)
        glDrawArrays(mode, first, count);
}

so_default_dynlib default_dynlib[] = {
        { "_ctype_", (uintptr_t)&BIONIC_ctype_},
        { "_tolower_tab_", (uintptr_t)&BIONIC_tolower_tab_},
        { "_toupper_tab_", (uintptr_t)&BIONIC_toupper_tab_},
        { "__aeabi_atexit", (uintptr_t)&__aeabi_atexit },
        { "__aeabi_d2f", (uintptr_t)&__aeabi_d2f },
        { "__aeabi_d2iz", (uintptr_t)&__aeabi_d2iz },
        { "__aeabi_d2uiz", (uintptr_t)&__aeabi_d2uiz },
        { "__aeabi_dadd", (uintptr_t)&__aeabi_dadd },
        { "__aeabi_dcmpeq", (uintptr_t)&__aeabi_dcmpeq },
        { "__aeabi_dcmpge", (uintptr_t)&__aeabi_dcmpge },
        { "__aeabi_dcmpgt", (uintptr_t)&__aeabi_dcmpgt },
        { "__aeabi_dcmple", (uintptr_t)&__aeabi_dcmple },
        { "__aeabi_dcmplt", (uintptr_t)&__aeabi_dcmplt },
        { "__aeabi_ddiv", (uintptr_t)&__aeabi_ddiv },
        { "__aeabi_dmul", (uintptr_t)&__aeabi_dmul },
        { "__aeabi_dsub", (uintptr_t)&__aeabi_dsub },
        { "__aeabi_f2d", (uintptr_t)&__aeabi_f2d },
        { "__aeabi_f2iz", (uintptr_t)&__aeabi_f2iz },
        { "__aeabi_fadd", (uintptr_t)&__aeabi_fadd },
        { "__aeabi_fcmpeq", (uintptr_t)&__aeabi_fcmpeq },
        { "__aeabi_fcmpge", (uintptr_t)&__aeabi_fcmpge },
        { "__aeabi_fcmpgt", (uintptr_t)&__aeabi_fcmpgt },
        { "__aeabi_fcmple", (uintptr_t)&__aeabi_fcmple },
        { "__aeabi_fcmplt", (uintptr_t)&__aeabi_fcmplt },
        { "__aeabi_fcmpun", (uintptr_t)&__aeabi_fcmpun },
        { "__aeabi_fdiv", (uintptr_t)&__aeabi_fdiv },
        { "__aeabi_fmul", (uintptr_t)&__aeabi_fmul },
        { "__aeabi_fsub", (uintptr_t)&__aeabi_fsub },
        { "__aeabi_i2d", (uintptr_t)&__aeabi_i2d },
        { "__aeabi_i2f", (uintptr_t)&__aeabi_i2f },
        { "__aeabi_idiv", (uintptr_t)&__aeabi_idiv },
        { "__aeabi_idivmod", (uintptr_t)&__aeabi_idivmod },
        { "__aeabi_ldivmod", (uintptr_t)&__aeabi_ldivmod },
        { "__aeabi_lmul", (uintptr_t)&__aeabi_lmul },
        { "__aeabi_ui2d", (uintptr_t)&__aeabi_ui2d },
        { "__aeabi_ui2f", (uintptr_t)&__aeabi_ui2f },
        { "__aeabi_uidiv", (uintptr_t)&__aeabi_uidiv },
        { "__aeabi_uidivmod", (uintptr_t)&__aeabi_uidivmod },
        { "__android_log_write", (uintptr_t)&__android_log_write },
        { "__cxa_guard_acquire", (uintptr_t)&__cxa_guard_acquire },
        { "__cxa_guard_release", (uintptr_t)&__cxa_guard_release },
        // { "__cxa_pure_virtual", (uintptr_t)&__cxa_pure_virtual },
        { "__dso_handle", (uintptr_t)&__dso_handle },
        { "__sF", (uintptr_t)&__sF_fake },
        { "__stack_chk_fail", (uintptr_t)&__stack_chk_fail },
        { "__stack_chk_guard", (uintptr_t)&__stack_chk_guard_fake },
        { "abort", (uintptr_t)&abort },
        { "acos", (uintptr_t)&acos },
        { "acosf", (uintptr_t)&acosf },
        { "asin", (uintptr_t)&asin },
        { "asinf", (uintptr_t)&asinf },
        { "atan", (uintptr_t)&atan },
        { "atan2", (uintptr_t)&atan2 },
        { "atan2f", (uintptr_t)&atan2f },
        { "atanf", (uintptr_t)&atanf },
        { "atoi", (uintptr_t)&atoi },
        { "ceilf", (uintptr_t)&ceilf },
        // { "chdir", (uintptr_t)&chdir },
        // { "close", (uintptr_t)&close },
        // { "connect", (uintptr_t)&connect },
        { "cos", (uintptr_t)&cos },
        { "cosf", (uintptr_t)&cosf },
        { "exit", (uintptr_t)&exit },
        { "expf", (uintptr_t)&expf },
        { "fclose", (uintptr_t)&fclose },
        { "fgetc", (uintptr_t)&fgetc },
        { "floor", (uintptr_t)&floor },
        { "floorf", (uintptr_t)&floorf },
        { "fmod", (uintptr_t)&fmod },
        { "fmodf", (uintptr_t)&fmodf },
        { "fopen", (uintptr_t)&fopen_hook },
        { "fprintf", (uintptr_t)&fprintf },
        { "fread", (uintptr_t)&fread },
        { "free", (uintptr_t)&free },
        { "fseek", (uintptr_t)&fseek },
        { "ftell", (uintptr_t)&ftell },
        { "fwrite", (uintptr_t)&fwrite },
        // { "gethostbyname", (uintptr_t)&gethostbyname },
        { "gettimeofday", (uintptr_t)&gettimeofday },
        { "glActiveTexture", (uintptr_t)&glActiveTexture },
        { "glAlphaFunc", (uintptr_t)&glAlphaFunc },
        { "glBindBuffer", (uintptr_t)&glBindBuffer },
        { "glBindTexture", (uintptr_t)&glBindTexture },
        { "glBlendFunc", (uintptr_t)&glBlendFunc },
        { "glBufferData", (uintptr_t)&glBufferData },
        { "glBufferSubData", (uintptr_t)&glBufferSubData },
        { "glClear", (uintptr_t)&glClear },
        { "glClearColor", (uintptr_t)&glClearColor },
        { "glClearDepthf", (uintptr_t)&glClearDepthf },
        { "glClientActiveTexture", (uintptr_t)&glClientActiveTexture },
        { "glClipPlanef", (uintptr_t)&glClipPlanef },
        { "glColor4f", (uintptr_t)&glColor4f },
        { "glColor4ub", (uintptr_t)&glColor4ub },
        { "glColorMask", (uintptr_t)&glColorMask },
        { "glColorPointer", (uintptr_t)&glColorPointer },
        { "glCompressedTexImage2D", (uintptr_t)&glCompressedTexImage2D },
        { "glCopyTexImage2D", (uintptr_t)&ret0 },
        { "glCullFace", (uintptr_t)&glCullFace },
        { "glDeleteBuffers", (uintptr_t)&glDeleteBuffers },
        { "glDeleteTextures", (uintptr_t)&glDeleteTextures },
        { "glDepthFunc", (uintptr_t)&glDepthFunc },
        { "glDepthMask", (uintptr_t)&glDepthMask },
        { "glDepthRangef", (uintptr_t)&glDepthRangef },
        { "glDisable", (uintptr_t)&glDisable },
        { "glDisableClientState", (uintptr_t)&glDisableClientState },
        { "glDrawArrays", (uintptr_t)&glDrawArraysHook },
        { "glDrawElements", (uintptr_t)&glDrawElementsHook },
        { "glEnable", (uintptr_t)&glEnable },
        { "glEnableClientState", (uintptr_t)&glEnableClientState },
        { "glFogf", (uintptr_t)&glFogf },
        { "glFogfv", (uintptr_t)&glFogfv },
        { "glFrontFace", (uintptr_t)&glFrontFace },
        { "glFrustumf", (uintptr_t)&glFrustumf },
        { "glGenBuffers", (uintptr_t)&glGenBuffers },
        { "glGenTextures", (uintptr_t)&glGenTextures },
        { "glGetBooleanv", (uintptr_t)&glGetBooleanv },
        { "glGetError", (uintptr_t)&glGetError },
        { "glGetFloatv", (uintptr_t)&glGetFloatv },
        { "glGetIntegerv", (uintptr_t)&glGetIntegerv },
        { "glGetPointerv", (uintptr_t)&glGetPointerv },
        { "glGetString", (uintptr_t)&glGetString },
        { "glGetTexEnviv", (uintptr_t)&glGetTexEnviv },
        { "glHint", (uintptr_t)&glHint },
        { "glIsEnabled", (uintptr_t)&glIsEnabled },
        { "glLightModelfv", (uintptr_t)&glLightModelfv },
        { "glLightf", (uintptr_t)&ret0 },
        { "glLightfv", (uintptr_t)&glLightfv },
        { "glLineWidth", (uintptr_t)&glLineWidth },
        { "glLoadIdentity", (uintptr_t)&glLoadIdentity },
        { "glLoadMatrixf", (uintptr_t)&glLoadMatrixf },
        { "glMaterialf", (uintptr_t)&ret0 },
        { "glMaterialfv", (uintptr_t)&glMaterialfv },
        { "glMatrixMode", (uintptr_t)&glMatrixMode },
        { "glMultMatrixf", (uintptr_t)&glMultMatrixf },
        { "glNormal3f", (uintptr_t)&glNormal3f },
        { "glNormalPointer", (uintptr_t)&glNormalPointer },
        { "glOrthox", (uintptr_t)&glOrthox },
        { "glPixelStorei", (uintptr_t)&glPixelStorei },
        { "glPointParameterf", (uintptr_t)&ret0 },
        { "glPointSize", (uintptr_t)&glPointSize },
        { "glPolygonOffset", (uintptr_t)&glPolygonOffset },
        { "glPopMatrix", (uintptr_t)&glPopMatrix },
        { "glPushMatrix", (uintptr_t)&glPushMatrix },
        { "glReadPixels", (uintptr_t)&glReadPixels },
        { "glRotatef", (uintptr_t)&glRotatef },
        { "glScalef", (uintptr_t)&glScalef },
        { "glScissor", (uintptr_t)&glScissor },
        { "glShadeModel", (uintptr_t)&ret0 },
        { "glStencilFunc", (uintptr_t)&glStencilFunc },
        { "glStencilMask", (uintptr_t)&glStencilMask },
        { "glStencilOp", (uintptr_t)&glStencilOp },
        { "glTexCoordPointer", (uintptr_t)&glTexCoordPointer },
        { "glTexEnvf", (uintptr_t)&glTexEnvf },
        { "glTexEnvfv", (uintptr_t)&glTexEnvfv },
        { "glTexEnvi", (uintptr_t)&glTexEnvi },
        { "glTexImage2D", (uintptr_t)&glTexImage2D },
        { "glTexParameterf", (uintptr_t)&glTexParameterf },
        { "glTexParameteri", (uintptr_t)&glTexParameteri },
        { "glTexParameterx", (uintptr_t)&glTexParameterx },
        { "glTexSubImage2D", (uintptr_t)&glTexSubImage2D },
        { "glTranslatef", (uintptr_t)&glTranslatef },
        { "glVertexPointer", (uintptr_t)&glVertexPointer },
        { "glViewport", (uintptr_t)&glViewport },
        { "logf", (uintptr_t)&logf },
        { "lrand48", (uintptr_t)&lrand48 },
        { "malloc", (uintptr_t)&malloc },
        { "memcmp", (uintptr_t)&memcmp },
        { "memcpy", (uintptr_t)&sceClibMemcpy },
        { "memmove", (uintptr_t)&sceClibMemmove },
        { "memset", (uintptr_t)&sceClibMemset },
        // { "nanosleep", (uintptr_t)&nanosleep },
        { "pow", (uintptr_t)&pow },
        { "powf", (uintptr_t)&powf },
        { "printf", (uintptr_t)&printf },
        { "puts", (uintptr_t)&puts },
        { "pthread_attr_destroy", (uintptr_t)&ret0 },
        { "pthread_attr_init", (uintptr_t)&ret0 },
        { "pthread_attr_setdetachstate", (uintptr_t)&ret0 },
        { "pthread_attr_setstacksize", (uintptr_t)&ret0 },
        { "pthread_cond_init", (uintptr_t)&pthread_cond_init_fake},
        { "pthread_cond_broadcast", (uintptr_t)&pthread_cond_broadcast_fake},
        { "pthread_cond_wait", (uintptr_t)&pthread_cond_wait_fake},
        { "pthread_cond_signal", (uintptr_t)&pthread_cond_signal_fake},
        { "pthread_cond_destroy", (uintptr_t)&pthread_cond_destroy_fake},
        { "pthread_cond_timedwait", (uintptr_t)&pthread_cond_timedwait_fake},
        { "pthread_cond_timedwait_relative_np", (uintptr_t)&pthread_cond_timedwait_relative_np_fake}, // FIXME
        { "pthread_create", (uintptr_t)&pthread_create_fake },
        { "pthread_getschedparam", (uintptr_t)&pthread_getschedparam },
        { "pthread_getspecific", (uintptr_t)&pthread_getspecific },
        { "pthread_key_create", (uintptr_t)&pthread_key_create },
        { "pthread_key_delete", (uintptr_t)&pthread_key_delete },
        { "pthread_mutex_destroy", (uintptr_t)&pthread_mutex_destroy_fake },
        { "pthread_mutex_init", (uintptr_t)&pthread_mutex_init_fake },
        { "pthread_mutex_trylock", (uintptr_t)&pthread_mutex_trylock_fake },
        { "pthread_mutex_lock", (uintptr_t)&pthread_mutex_lock_fake },
        { "pthread_mutex_unlock", (uintptr_t)&pthread_mutex_unlock_fake },
        { "pthread_mutexattr_destroy", (uintptr_t)&pthread_mutexattr_destroy},
        { "pthread_mutexattr_init", (uintptr_t)&pthread_mutexattr_init},
        { "pthread_mutexattr_settype", (uintptr_t)&pthread_mutexattr_settype},
        { "pthread_once", (uintptr_t)&pthread_once_fake },
        { "pthread_self", (uintptr_t)&pthread_self },
        { "pthread_setname_np", (uintptr_t)&ret0 },
        { "pthread_getschedparam", (uintptr_t)&pthread_getschedparam },
        { "pthread_setschedparam", (uintptr_t)&pthread_setschedparam },
        { "pthread_setspecific", (uintptr_t)&pthread_setspecific },
        // { "recv", (uintptr_t)&recv },
        { "remove", (uintptr_t)&remove },
        { "rename", (uintptr_t)&rename },
        { "sin", (uintptr_t)&sin },
        { "sinf", (uintptr_t)&sinf },
        { "snprintf", (uintptr_t)&snprintf },
        // { "socket", (uintptr_t)&socket },
        { "sprintf", (uintptr_t)&sprintf },
        { "sqrt", (uintptr_t)&sqrt },
        { "sqrtf", (uintptr_t)&sqrtf },
        { "srand48", (uintptr_t)&srand48 },
        { "sscanf", (uintptr_t)&sscanf },
        { "strcasecmp", (uintptr_t)&strcasecmp },
        { "strcat", (uintptr_t)&strcat },
        { "strchr", (uintptr_t)&strchr },
        { "strcmp", (uintptr_t)&strcmp },
        { "strcpy", (uintptr_t)&strcpy },
        { "strdup", (uintptr_t)&strdup },
        { "strlen", (uintptr_t)&strlen },
        { "strncmp", (uintptr_t)&strncmp },
        { "strpbrk", (uintptr_t)&strpbrk },
        { "strstr", (uintptr_t)&strstr },
        { "strtod", (uintptr_t)&strtod },
        { "tan", (uintptr_t)&tan },
        { "tanf", (uintptr_t)&tanf },
        { "uname", (uintptr_t)&uname_fake },
        { "vsprintf", (uintptr_t)&vsprintf },
        { "wcscmp", (uintptr_t)&wcscmp },
        { "wcscpy", (uintptr_t)&wcscpy },
        { "wcslen", (uintptr_t)&wcslen },
        { "wmemcpy", (uintptr_t)&wmemcpy },
        { "glCompileShader", (uintptr_t)&glCompileShader },
        { "glDeleteRenderbuffers", (uintptr_t)&glDeleteRenderbuffers },
        { "glClearStencil", (uintptr_t)&glClearStencil },
        { "glBindRenderbuffer", (uintptr_t)&glBindRenderbuffer },
        { "glDisableVertexAttribArray", (uintptr_t)&glDisableVertexAttribArray },
        { "glUniform1f", (uintptr_t)&glUniform1f },
        { "glUniform3fv", (uintptr_t)&glUniform3fv },
        { "glUniform2fv", (uintptr_t)&glUniform2fv },
        { "glUniform2iv", (uintptr_t)&glUniform2iv },
        { "glUniform4iv", (uintptr_t)&glUniform4iv },
        { "glUseProgram", (uintptr_t)&glUseProgram },
        { "glGenerateMipmap", (uintptr_t)&glGenerateMipmap },
        { "glGetAttribLocation", (uintptr_t)&glGetAttribLocation },
        { "glBlendEquation", (uintptr_t)&glBlendEquation },
        { "glCreateShader", (uintptr_t)&glCreateShader },
        { "glDeleteFramebuffers", (uintptr_t)&glDeleteFramebuffers },
        { "glFramebufferRenderbuffer", (uintptr_t)&glFramebufferRenderbuffer },
        { "glShaderSource", (uintptr_t)&glShaderSource },
        { "glGetUniformLocation", (uintptr_t)&glGetUniformLocation },
        { "glCheckFramebufferStatus", (uintptr_t)&glCheckFramebufferStatus },
        { "glCopyTexSubImage2D", (uintptr_t)&glCopyTexSubImage2D },
        { "glDeleteShader", (uintptr_t)&glDeleteShader },
        { "fcntl", (uintptr_t)&fcntl },
            { "clock", (uintptr_t)&clock},
        { "glAttachShader", (uintptr_t)&glAttachShader},
        { "bind", (uintptr_t)&bind},
        { "sysconf", (uintptr_t)&ret0},
        { "strcoll", (uintptr_t)&strcoll},
        { "modff", (uintptr_t)&modff},
        { "iswalpha", (uintptr_t)&iswalpha},
        { "difftime", (uintptr_t)&difftime},
        { "wcsncpy", (uintptr_t)&wcsncpy},
        { "strftime", (uintptr_t)&strftime},
        { "open", (uintptr_t)&open},
        { "glGetShaderiv", (uintptr_t)&glGetShaderiv},
        { "getenv", (uintptr_t)&getenv},
        { "system", (uintptr_t)&system},
        { "usleep", (uintptr_t)&usleep},
        { "glGetActiveUniform", (uintptr_t)&glGetActiveUniform},
        { "realloc", (uintptr_t)&realloc},
        { "glGetActiveAttrib", (uintptr_t)&glGetActiveAttrib},
        { "iswspace", (uintptr_t)&iswspace},
        { "glFramebufferTexture2D", (uintptr_t)&glFramebufferTexture2D},
        { "putchar", (uintptr_t)&putchar},
        { "tmpnam", (uintptr_t)&tmpnam},
        { "ceil", (uintptr_t)&ceil},
        { "cosh", (uintptr_t)&cosh},
        { "freeaddrinfo", (uintptr_t)&freeaddrinfo},
        { "strtoul", (uintptr_t)&strtoul},
        { "strrchr", (uintptr_t)&strrchr},
        { "glDeleteProgram", (uintptr_t)&glDeleteProgram},
        { "glGenRenderbuffers", (uintptr_t)&glGenRenderbuffers},
        { "iswlower", (uintptr_t)&iswlower},
        { "glUniformMatrix4fv", (uintptr_t)&glUniformMatrix4fv},
        { "tanh", (uintptr_t)&tanh},
        { "wmemmove", (uintptr_t)&wmemmove},
        { "glUniform1iv", (uintptr_t)&glUniform1iv},
        { "sendto", (uintptr_t)&sendto},
        { "lseek", (uintptr_t)&lseek},
        { "gmtime", (uintptr_t)&gmtime},
        { "read", (uintptr_t)&read},
        //{ "glBlendColor", (uintptr_t)&glBlendColor},
        { "glEnableVertexAttribArray", (uintptr_t)&glEnableVertexAttribArray},
        { "select", (uintptr_t)&select},
        { "strncasecmp", (uintptr_t)&strncasecmp},
        { "glGenFramebuffers", (uintptr_t)&glGenFramebuffers},
        { "glVertexAttrib4f", (uintptr_t)&glVertexAttrib4f},
        //{ "__android_log_print", (uintptr_t)&__android_log_print},
        { "frexpf", (uintptr_t)&frexpf},
        { "freopen", (uintptr_t)&freopen},
        { "setjmp", (uintptr_t)&setjmp},
        { "sched_yield", (uintptr_t)&sched_yield},
        { "mktime", (uintptr_t)&mktime},
        { "mmap", (uintptr_t)&mmap},
        { "setsockopt", (uintptr_t)&setsockopt},
        { "glGetProgramInfoLog", (uintptr_t)&glGetProgramInfoLog},
        { "strncat", (uintptr_t)&strncat},
        //{ "gethostname", (uintptr_t)&gethostname},
        { "towlower", (uintptr_t)&towlower},
        { "tmpfile", (uintptr_t)&tmpfile},
        { "glRenderbufferStorage", (uintptr_t)&glRenderbufferStorage},
        { "time", (uintptr_t)&time},
        { "iswxdigit", (uintptr_t)&iswxdigit},
        { "strtok", (uintptr_t)&strtok},
        { "glGetProgramiv", (uintptr_t)&glGetProgramiv},
        { "write", (uintptr_t)&write},
        { "setvbuf", (uintptr_t)&setvbuf},
        //{ "_ZnajRKSt9nothrow_t", (uintptr_t)&_ZnajRKSt9nothrow_t},
        { "longjmp", (uintptr_t)&longjmp},
        { "__aeabi_d2lz", (uintptr_t)&__aeabi_d2lz},
        { "iswprint", (uintptr_t)&iswprint},
        { "wmemcmp", (uintptr_t)&wmemcmp},
        { "localtime", (uintptr_t)&localtime},
        { "close", (uintptr_t)&close},
        { "fputc", (uintptr_t)&fputc},
        { "glCreateProgram", (uintptr_t)&glCreateProgram},
        { "fgets", (uintptr_t)&fgets},
        { "ferror", (uintptr_t)&ferror},
        { "iswupper", (uintptr_t)&iswupper},
        { "strncpy", (uintptr_t)&strncpy},
        { "glSampleCoverage", (uintptr_t)&ret0},
        { "__aeabi_uldivmod", (uintptr_t)&__aeabi_uldivmod},
        { "fputs", (uintptr_t)&fputs},
        { "glUniform1i", (uintptr_t)&glUniform1i},
        { "iswcntrl", (uintptr_t)&iswcntrl},
        { "chdir", (uintptr_t)&chdir},
        { "unlink", (uintptr_t)&unlink},
        { "fsetpos", (uintptr_t)&fsetpos},
        { "sinh", (uintptr_t)&sinh},
        { "fflush", (uintptr_t)&fflush},
        { "wmemset", (uintptr_t)&wmemset},
        { "iswpunct", (uintptr_t)&iswpunct},
        { "qsort", (uintptr_t)&qsort},
        { "munmap", (uintptr_t)&munmap},
        { "iswdigit", (uintptr_t)&iswdigit},
        { "nanosleep", (uintptr_t)&nanosleep_hook},
        //{ "glCompressedTexSubImage2D", (uintptr_t)&glCompressedTexSubImage2D},
        { "strcspn", (uintptr_t)&strcspn},
        { "__aeabi_ul2d", (uintptr_t)&__aeabi_ul2d},
        { "vsnprintf", (uintptr_t)&vsnprintf},
        { "recvfrom", (uintptr_t)&recvfrom},
        { "socket", (uintptr_t)&socket},
        { "memchr", (uintptr_t)&memchr},
        { "glVertexAttribPointer", (uintptr_t)&glVertexAttribPointer},
        { "glFlush", (uintptr_t)&glFlush},
        { "glBindFramebuffer", (uintptr_t)&glBindFramebuffer},
        { "glLinkProgram", (uintptr_t)&glLinkProgram},
        { "glUniform3iv", (uintptr_t)&glUniform3iv},
        { "fgetpos", (uintptr_t)&fgetpos},
        { "fstat", (uintptr_t)&fstat},
        { "glUniform1fv", (uintptr_t)&glUniform1fv},
        { "glGetShaderInfoLog", (uintptr_t)&glGetShaderInfoLog},
        //{ "__errno", (uintptr_t)&__errno},
        { "ungetc", (uintptr_t)&ungetc},
        //{ "__srget", (uintptr_t)&__srget},
        { "getc", (uintptr_t)&getc},
        { "glUniform4fv", (uintptr_t)&glUniform4fv},
        { "strerror", (uintptr_t)&strerror},
        { "setlocale", (uintptr_t)&ret0},
        { "swprintf", (uintptr_t)&swprintf},
        { "fscanf", (uintptr_t)&fscanf},
        { "ldexpf", (uintptr_t)&ldexpf},
        { "towupper", (uintptr_t)&towupper},
        { "log10", (uintptr_t)&log10},
        { "getaddrinfo", (uintptr_t)&getaddrinfo},
        { "putc", (uintptr_t)&putc},
        { "pthread_join", (uintptr_t)&pthread_join},
};

void resolve_imports(so_module* mod) {
    __sF_fake[0] = *stdin;
    __sF_fake[1] = *stdout;
    __sF_fake[2] = *stderr;
    so_resolve(mod, default_dynlib, sizeof(default_dynlib), 0);
}
