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

#include <SLES/OpenSLES.h>
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
#include "reimpl/env.h"
#include "reimpl/mem.h"
#include <sys/socket.h>
#include <netdb.h>
#include <wchar.h>
#include <wctype.h>
#include "reimpl/pthr.h"

#if 0
int gethostname(char *name, size_t len) { return 0; }
#endif
int nanosleep(const struct timespec *rqtp, struct timespec *rmtp);

extern void *__aeabi_atexit;
extern void *__cxa_pure_virtual;
extern void *__stack_chk_fail;
extern void *__stack_chk_guard;

extern void * __aeabi_d2f;
extern void * __aeabi_d2iz;
extern void * __aeabi_d2uiz;
extern void * __aeabi_dcmpge;
extern void * __aeabi_dcmple;
extern void * __aeabi_fcmpun;
extern void * __cxa_guard_acquire;
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
extern void * __aeabi_d2lz;
extern void * __aeabi_ul2d;
extern void * __cxa_guard_release;
extern void * _Znwj;
extern void * _ZSt7nothrow;
extern void * _ZnajRKSt9nothrow_t;
extern void * __srget;

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

int pthread_cond_timedwait_relative_np_soloader() {
    log_error("Called pthread_cond_timedwait_relative_np_soloader !!!");
    return 0;
}

int uname_fake(void *buf) {
    strcpy(buf + 195, "1.0");
    return 0;
}

int __errno_fake() {
    log_error("Called __errno_fake !!!");
    return __errno();
}

GLint glGetUniformLocation_hook(GLuint program, const GLchar *name) {
	if (!strcmp(name, "texture"))
		return glGetUniformLocation(program, "_texture");
	return glGetUniformLocation(program, name);
}

void glGetActiveUniform_hook(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) {
	glGetActiveUniform(program, index, bufSize, length, size, type, name);
	if (!strcmp(name, "BoneMatrices")) {
		*type = GL_FLOAT_MAT4;
		*size = 48;
	}
	if (!strcmp(name, "_texture")) {
		strcpy(name, "texture");
		if (length)
			*length = *length - 1;
	}
}

so_default_dynlib default_dynlib[] = {
        { "__aeabi_atexit", (uintptr_t)&__aeabi_atexit },
        { "__aeabi_d2f", (uintptr_t)&__aeabi_d2f },
        { "__aeabi_d2iz", (uintptr_t)&__aeabi_d2iz },
        { "__aeabi_d2lz", (uintptr_t)&__aeabi_d2lz},
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
        { "__aeabi_ul2d", (uintptr_t)&__aeabi_ul2d},
        { "__aeabi_uldivmod", (uintptr_t)&__aeabi_uldivmod},
        { "__android_log_print", (uintptr_t)&android_log_print },
        { "__android_log_write", (uintptr_t)&android_log_write },
        { "__cxa_guard_acquire", (uintptr_t)&__cxa_guard_acquire },
        { "__cxa_guard_release", (uintptr_t)&__cxa_guard_release },
        { "__cxa_pure_virtual", (uintptr_t)&__cxa_pure_virtual },
        { "__dso_handle", (uintptr_t)&__dso_handle },
        { "__errno", (uintptr_t)&__errno_fake },
        { "__sF", (uintptr_t)&__sF_fake },
        { "__srget", (uintptr_t)&__srget },
        { "__stack_chk_fail", (uintptr_t)&__stack_chk_fail },
        { "__stack_chk_guard", (uintptr_t)&__stack_chk_guard },
        { "_ctype_", (uintptr_t)&BIONIC_ctype_},
        { "_tolower_tab_", (uintptr_t)&BIONIC_tolower_tab_},
        { "_toupper_tab_", (uintptr_t)&BIONIC_toupper_tab_},
        { "_ZnajRKSt9nothrow_t", (uintptr_t)&_ZnajRKSt9nothrow_t },
        { "_ZSt7nothrow", (uintptr_t)&_ZSt7nothrow },
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
        { "bind", (uintptr_t)&bind},
        { "ceil", (uintptr_t)&ceil},
        { "ceilf", (uintptr_t)&ceilf },
        { "chdir", (uintptr_t)&chdir},
        { "clock", (uintptr_t)&clock },
        { "close", (uintptr_t)&close},
        { "cos", (uintptr_t)&cos },
        { "cosf", (uintptr_t)&cosf },
        { "cosh", (uintptr_t)&cosh},
        { "difftime", (uintptr_t)&difftime},
        { "exit", (uintptr_t)&exit },
        { "expf", (uintptr_t)&expf },
        { "fclose", (uintptr_t)&fclose },
        { "fcntl", (uintptr_t)&fcntl_soloader },
        { "ferror", (uintptr_t)&ferror},
        { "fflush", (uintptr_t)&fflush},
        { "fgetc", (uintptr_t)&fgetc },
        { "fgetpos", (uintptr_t)&fgetpos},
        { "fgets", (uintptr_t)&fgets},
        { "floor", (uintptr_t)&floor },
        { "floorf", (uintptr_t)&floorf },
        { "fmod", (uintptr_t)&fmod },
        { "fmodf", (uintptr_t)&fmodf },
        { "fopen", (uintptr_t)&fopen_soloader },
        { "fprintf", (uintptr_t)&fprintf },
        { "fputc", (uintptr_t)&fputc},
        { "fputs", (uintptr_t)&fputs},
        { "fread", (uintptr_t)&fread },
        { "free", (uintptr_t)&free },
        { "freeaddrinfo", (uintptr_t)&freeaddrinfo},
        { "freopen", (uintptr_t)&freopen},
        { "frexpf", (uintptr_t)&frexpf},
        { "fscanf", (uintptr_t)&fscanf},
        { "fseek", (uintptr_t)&fseek },
        { "fsetpos", (uintptr_t)&fsetpos},
        { "fstat", (uintptr_t)&fstat_soloader},
        { "ftell", (uintptr_t)&ftell },
        { "fwrite", (uintptr_t)&fwrite },
        { "getaddrinfo", (uintptr_t)&getaddrinfo},
        { "getc", (uintptr_t)&getc},
        { "getenv", (uintptr_t)&getenv_soloader },
        { "gethostname", (uintptr_t)&gethostname },
        { "gettimeofday", (uintptr_t)&gettimeofday },
        { "glActiveTexture", (uintptr_t)&glActiveTexture },
        { "glAlphaFunc", (uintptr_t)&glAlphaFunc },
        { "glAttachShader", (uintptr_t)&glAttachShader},
        { "glBindBuffer", (uintptr_t)&glBindBuffer },
        { "glBindFramebuffer", (uintptr_t)&glBindFramebuffer},
        { "glBindRenderbuffer", (uintptr_t)&glBindRenderbuffer },
        { "glBindTexture", (uintptr_t)&glBindTexture },
        { "glBlendColor", (uintptr_t)&ret0 },
        { "glBlendEquation", (uintptr_t)&glBlendEquation },
        { "glBlendFunc", (uintptr_t)&glBlendFunc },
        { "glBufferData", (uintptr_t)&glBufferData },
        { "glBufferSubData", (uintptr_t)&glBufferSubData },
        { "glCheckFramebufferStatus", (uintptr_t)&glCheckFramebufferStatus },
        { "glClear", (uintptr_t)&glClear },
        { "glClearColor", (uintptr_t)&glClearColor },
        { "glClearDepthf", (uintptr_t)&glClearDepthf },
        { "glClearStencil", (uintptr_t)&glClearStencil },
        { "glClientActiveTexture", (uintptr_t)&glClientActiveTexture },
        { "glClipPlanef", (uintptr_t)&glClipPlanef },
        { "glColor4f", (uintptr_t)&glColor4f },
        { "glColor4ub", (uintptr_t)&glColor4ub },
        { "glColorMask", (uintptr_t)&glColorMask },
        { "glColorPointer", (uintptr_t)&glColorPointer },
        { "glCompileShader", (uintptr_t)&ret0 },
        { "glCompressedTexImage2D", (uintptr_t)&glCompressedTexImage2D },
        { "glCompressedTexSubImage2D", (uintptr_t)&ret0},
        { "glCopyTexImage2D", (uintptr_t)&ret0 },
        { "glCopyTexSubImage2D", (uintptr_t)&glCopyTexSubImage2D },
        { "glCreateProgram", (uintptr_t)&glCreateProgram},
        { "glCreateShader", (uintptr_t)&glCreateShader },
        { "glCullFace", (uintptr_t)&glCullFace },
        { "glDeleteBuffers", (uintptr_t)&glDeleteBuffers },
        { "glDeleteFramebuffers", (uintptr_t)&glDeleteFramebuffers },
        { "glDeleteProgram", (uintptr_t)&glDeleteProgram},
        { "glDeleteRenderbuffers", (uintptr_t)&glDeleteRenderbuffers },
        { "glDeleteShader", (uintptr_t)&glDeleteShader },
        { "glDeleteTextures", (uintptr_t)&glDeleteTextures },
        { "glDepthFunc", (uintptr_t)&glDepthFunc },
        { "glDepthMask", (uintptr_t)&glDepthMask },
        { "glDepthRangef", (uintptr_t)&glDepthRangef },
        { "glDisable", (uintptr_t)&glDisable },
        { "glDisableClientState", (uintptr_t)&glDisableClientState },
        { "glDisableVertexAttribArray", (uintptr_t)&glDisableVertexAttribArray },
        { "glDrawArrays", (uintptr_t)&glDrawArraysHook },
        { "glDrawElements", (uintptr_t)&glDrawElementsHook },
        { "glEnable", (uintptr_t)&glEnable },
        { "glEnableClientState", (uintptr_t)&glEnableClientState },
        { "glEnableVertexAttribArray", (uintptr_t)&glEnableVertexAttribArray},
        { "glFlush", (uintptr_t)&glFlush},
        { "glFogf", (uintptr_t)&glFogf },
        { "glFogfv", (uintptr_t)&glFogfv },
        { "glFramebufferRenderbuffer", (uintptr_t)&glFramebufferRenderbuffer },
        { "glFramebufferTexture2D", (uintptr_t)&glFramebufferTexture2D},
        { "glFrontFace", (uintptr_t)&glFrontFace },
        { "glFrustumf", (uintptr_t)&glFrustumf },
        { "glGenBuffers", (uintptr_t)&glGenBuffers },
        { "glGenerateMipmap", (uintptr_t)&glGenerateMipmap },
        { "glGenFramebuffers", (uintptr_t)&glGenFramebuffers},
        { "glGenRenderbuffers", (uintptr_t)&glGenRenderbuffers},
        { "glGenTextures", (uintptr_t)&glGenTextures },
        { "glGetActiveAttrib", (uintptr_t)&glGetActiveAttrib},
        { "glGetActiveUniform", (uintptr_t)&glGetActiveUniform_hook},
        { "glGetAttribLocation", (uintptr_t)&glGetAttribLocation },
        { "glGetBooleanv", (uintptr_t)&glGetBooleanv },
        { "glGetError", (uintptr_t)&glGetError },
        { "glGetFloatv", (uintptr_t)&glGetFloatv },
        { "glGetIntegerv", (uintptr_t)&glGetIntegerv },
        { "glGetPointerv", (uintptr_t)&glGetPointerv },
        { "glGetProgramInfoLog", (uintptr_t)&glGetProgramInfoLog},
        { "glGetProgramiv", (uintptr_t)&glGetProgramiv},
        { "glGetShaderInfoLog", (uintptr_t)&glGetShaderInfoLog},
        { "glGetShaderiv", (uintptr_t)&glGetShaderiv},
        { "glGetString", (uintptr_t)&glGetString },
        { "glGetTexEnviv", (uintptr_t)&glGetTexEnviv },
        { "glGetUniformLocation", (uintptr_t)&glGetUniformLocation_hook },
        { "glHint", (uintptr_t)&glHint },
        { "glIsEnabled", (uintptr_t)&glIsEnabled },
        { "glLightf", (uintptr_t)&ret0 },
        { "glLightfv", (uintptr_t)&glLightfv },
        { "glLightModelfv", (uintptr_t)&glLightModelfv },
        { "glLineWidth", (uintptr_t)&glLineWidth },
        { "glLinkProgram", (uintptr_t)&glLinkProgram},
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
        { "glRenderbufferStorage", (uintptr_t)&glRenderbufferStorage},
        { "glRotatef", (uintptr_t)&glRotatef },
        { "glSampleCoverage", (uintptr_t)&ret0},
        { "glScalef", (uintptr_t)&glScalef },
        { "glScissor", (uintptr_t)&glScissor },
        { "glShadeModel", (uintptr_t)&ret0 },
        { "glShaderSource", (uintptr_t)&glShaderSourceHook },
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
        { "glUniform1f", (uintptr_t)&glUniform1f },
        { "glUniform1fv", (uintptr_t)&glUniform1fv},
        { "glUniform1i", (uintptr_t)&glUniform1i},
        { "glUniform1iv", (uintptr_t)&glUniform1iv},
        { "glUniform2fv", (uintptr_t)&glUniform2fv },
        { "glUniform2iv", (uintptr_t)&glUniform2iv },
        { "glUniform3fv", (uintptr_t)&glUniform3fv },
        { "glUniform3iv", (uintptr_t)&glUniform3iv},
        { "glUniform4fv", (uintptr_t)&glUniform4fv},
        { "glUniform4iv", (uintptr_t)&glUniform4iv },
        { "glUniformMatrix4fv", (uintptr_t)&glUniformMatrix4fv},
        { "glUseProgram", (uintptr_t)&glUseProgram },
        { "glVertexAttrib4f", (uintptr_t)&glVertexAttrib4f},
        { "glVertexAttribPointer", (uintptr_t)&glVertexAttribPointer},
        { "glVertexPointer", (uintptr_t)&glVertexPointer },
        { "glViewport", (uintptr_t)&glViewport },
        { "gmtime", (uintptr_t)&gmtime},
        { "iswalpha", (uintptr_t)&iswalpha},
        { "iswcntrl", (uintptr_t)&iswcntrl},
        { "iswdigit", (uintptr_t)&iswdigit},
        { "iswlower", (uintptr_t)&iswlower},
        { "iswprint", (uintptr_t)&iswprint},
        { "iswpunct", (uintptr_t)&iswpunct},
        { "iswspace", (uintptr_t)&iswspace},
        { "iswupper", (uintptr_t)&iswupper},
        { "iswxdigit", (uintptr_t)&iswxdigit},
        { "ldexpf", (uintptr_t)&ldexpf},
        { "localtime", (uintptr_t)&localtime},
        { "log10", (uintptr_t)&log10},
        { "logf", (uintptr_t)&logf },
        { "longjmp", (uintptr_t)&longjmp},
        { "lrand48", (uintptr_t)&lrand48 },
        { "lseek", (uintptr_t)&lseek},
        { "malloc", (uintptr_t)&malloc },
        { "memchr", (uintptr_t)&memchr},
        { "memcmp", (uintptr_t)&memcmp },
        { "memcpy", (uintptr_t)&memcpy },
        { "memmove", (uintptr_t)&memmove },
        { "memset", (uintptr_t)&memset },
        { "mktime", (uintptr_t)&mktime},
        { "mmap", (uintptr_t)&mmap},
        { "modff", (uintptr_t)&modff},
        { "munmap", (uintptr_t)&munmap},
        { "nanosleep", (uintptr_t)&nanosleep },
        { "open", (uintptr_t)&open_soloader },
        { "pow", (uintptr_t)&pow },
        { "powf", (uintptr_t)&powf },
        { "printf", (uintptr_t)&sceClibPrintf },
        { "pthread_attr_destroy", (uintptr_t)&pthread_attr_destroy_soloader },
        { "pthread_attr_init", (uintptr_t)&pthread_attr_init_soloader },
        { "pthread_attr_setdetachstate", (uintptr_t)&pthread_attr_setdetachstate_soloader },
        { "pthread_attr_setstacksize", (uintptr_t)&pthread_attr_setstacksize_soloader },
        { "pthread_cond_broadcast", (uintptr_t)&pthread_cond_broadcast_soloader},
        { "pthread_cond_destroy", (uintptr_t)&pthread_cond_destroy_soloader},
        { "pthread_cond_init", (uintptr_t)&pthread_cond_init_soloader},
        { "pthread_cond_signal", (uintptr_t)&pthread_cond_signal_soloader},
        { "pthread_cond_timedwait", (uintptr_t)&pthread_cond_timedwait_soloader},
        { "pthread_cond_timedwait_relative_np", (uintptr_t)&pthread_cond_timedwait_relative_np_soloader},
        { "pthread_cond_wait", (uintptr_t)&pthread_cond_wait_soloader},
        { "pthread_create", (uintptr_t)&pthread_create_soloader },
        { "pthread_getschedparam", (uintptr_t)&pthread_getschedparam_soloader },
        { "pthread_getschedparam", (uintptr_t)&pthread_getschedparam_soloader },
        { "pthread_getspecific", (uintptr_t)&pthread_getspecific },
        { "pthread_join", (uintptr_t)&pthread_join_soloader },
        { "pthread_key_create", (uintptr_t)&pthread_key_create },
        { "pthread_key_delete", (uintptr_t)&pthread_key_delete },
        { "pthread_mutex_destroy", (uintptr_t)&pthread_mutex_destroy_soloader },
        { "pthread_mutex_init", (uintptr_t)&pthread_mutex_init_soloader },
        { "pthread_mutex_lock", (uintptr_t)&pthread_mutex_lock_soloader },
        { "pthread_mutex_trylock", (uintptr_t)&pthread_mutex_trylock_soloader },
        { "pthread_mutex_unlock", (uintptr_t)&pthread_mutex_unlock_soloader },
        { "pthread_mutexattr_destroy", (uintptr_t)&pthread_mutexattr_destroy_soloader },
        { "pthread_mutexattr_init", (uintptr_t)&pthread_mutexattr_init_soloader },
        { "pthread_mutexattr_settype", (uintptr_t)&pthread_mutexattr_settype_soloader },
        { "pthread_once", (uintptr_t)&pthread_once_soloader },
        { "pthread_self", (uintptr_t)&pthread_self_soloader },
        { "pthread_setname_np", (uintptr_t)&pthread_setname_np_soloader },
        { "pthread_setschedparam", (uintptr_t)&pthread_setschedparam_soloader },
        { "pthread_setspecific", (uintptr_t)&pthread_setspecific },
        { "putc", (uintptr_t)&putc},
        { "putchar", (uintptr_t)&putchar},
        { "puts", (uintptr_t)&puts },
        { "qsort", (uintptr_t)&qsort},
        { "read", (uintptr_t)&read},
        { "realloc", (uintptr_t)&realloc},
        { "recvfrom", (uintptr_t)&recvfrom},
        { "remove", (uintptr_t)&remove },
        { "rename", (uintptr_t)&rename },
        { "sched_yield", (uintptr_t)&sched_yield},
        { "select", (uintptr_t)&select},
        { "sendto", (uintptr_t)&sendto},
        { "setjmp", (uintptr_t)&setjmp},
        { "setlocale", (uintptr_t)&ret0},
        { "setsockopt", (uintptr_t)&setsockopt},
        { "setvbuf", (uintptr_t)&setvbuf},
        { "sin", (uintptr_t)&sin },
        { "sinf", (uintptr_t)&sinf },
        { "sinh", (uintptr_t)&sinh},
        { "snprintf", (uintptr_t)&snprintf },
        { "socket", (uintptr_t)&socket},
        { "sprintf", (uintptr_t)&sprintf },
        { "sqrt", (uintptr_t)&sqrt },
        { "sqrtf", (uintptr_t)&sqrtf },
        { "srand48", (uintptr_t)&srand48 },
        { "sscanf", (uintptr_t)&sscanf },
        { "strcasecmp", (uintptr_t)&strcasecmp },
        { "strcat", (uintptr_t)&strcat },
        { "strchr", (uintptr_t)&strchr },
        { "strcmp", (uintptr_t)&strcmp },
        { "strcoll", (uintptr_t)&strcoll},
        { "strcpy", (uintptr_t)&strcpy },
        { "strcspn", (uintptr_t)&strcspn},
        { "strdup", (uintptr_t)&strdup },
        { "strerror", (uintptr_t)&strerror},
        { "strftime", (uintptr_t)&strftime},
        { "strlen", (uintptr_t)&strlen },
        { "strncasecmp", (uintptr_t)&strncasecmp},
        { "strncat", (uintptr_t)&strncat},
        { "strncmp", (uintptr_t)&strncmp },
        { "strncpy", (uintptr_t)&strncpy},
        { "strpbrk", (uintptr_t)&strpbrk },
        { "strrchr", (uintptr_t)&strrchr},
        { "strstr", (uintptr_t)&strstr },
        { "strtod", (uintptr_t)&strtod },
        { "strtok", (uintptr_t)&strtok},
        { "strtoul", (uintptr_t)&strtoul},
        { "swprintf", (uintptr_t)&swprintf},
        { "sysconf", (uintptr_t)&ret0},
        { "system", (uintptr_t)&system},
        { "tan", (uintptr_t)&tan },
        { "tanf", (uintptr_t)&tanf },
        { "tanh", (uintptr_t)&tanh},
        { "time", (uintptr_t)&time},
        { "tmpfile", (uintptr_t)&tmpfile},
        { "tmpnam", (uintptr_t)&tmpnam},
        { "towlower", (uintptr_t)&towlower},
        { "towupper", (uintptr_t)&towupper},
        { "uname", (uintptr_t)&uname_fake },
        { "ungetc", (uintptr_t)&ungetc},
        { "unlink", (uintptr_t)&unlink},
        { "usleep", (uintptr_t)&usleep},
        { "vsnprintf", (uintptr_t)&vsnprintf},
        { "vsprintf", (uintptr_t)&vsprintf },
        { "wcscmp", (uintptr_t)&wcscmp },
        { "wcscpy", (uintptr_t)&wcscpy },
        { "wcslen", (uintptr_t)&wcslen },
        { "wcsncpy", (uintptr_t)&wcsncpy},
        { "wmemcmp", (uintptr_t)&wmemcmp},
        { "wmemcpy", (uintptr_t)&wmemcpy },
        { "wmemmove", (uintptr_t)&wmemmove},
        { "wmemset", (uintptr_t)&wmemset},
        { "write", (uintptr_t)&write_soloader},
};

void resolve_imports(so_module* mod) {
    __sF_fake[0] = *stdin;
    __sF_fake[1] = *stdout;
    __sF_fake[2] = *stderr;
    so_resolve(mod, default_dynlib, sizeof(default_dynlib), 0);
}
