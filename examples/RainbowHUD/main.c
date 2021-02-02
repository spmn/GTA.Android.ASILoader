#include <android/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>


#define LOG_TAG "GTA RainbowHUD"

#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


typedef struct {
    char r, g, b, a;
} CRGBA;


void Redirect(void *src, void *dst, int thumb)
{
    unsigned int adjustedSrc = (unsigned int)(src);
    unsigned int adjustedDst = (unsigned int)(dst);
    char code[8];

    if (thumb) {
        adjustedSrc &= 0xFFFFFFFE;
        adjustedDst |= 0x00000001;       
        *(unsigned int *)&code[0] = 0xF000F8DF;  // ldr.w pc, [pc] ; +0x04
    } else {
        *(unsigned int *)&code[0] = 0xE51FF004;  // ldr pc, [pc, #-4] ; +0x04
    }

    *(unsigned int *)&code[4] = adjustedDst; // .word DEST
    memcpy((void *)adjustedSrc, code, sizeof(code));
    cacheflush(adjustedSrc, adjustedSrc + sizeof(code), 0);
}

__attribute__((target("arm")))
CRGBA *CRGBA__ctor_arm(CRGBA *this, __attribute__((unused)) char r, __attribute__((unused)) char g, __attribute__((unused)) char b, char a)
{
    // randimize colours
    this->r = rand();
    this->g = rand();
    this->b = rand();
    this->a = a;

    return this;
}

CRGBA *CRGBA__ctor_thumb(CRGBA *this, __attribute__((unused)) char r, __attribute__((unused)) char g, __attribute__((unused)) char b, char a)
{
    // randimize colours
    this->r = rand();
    this->g = rand();
    this->b = rand();
    this->a = a;

    return this;
}

void InitializeASI(void *pGTAHandle, void *pGTABaseAddress, const char *szGTASOLibFile, const char *szExternalASIDir)
{
    ALOGI("InitializeASI(pGTAHandle = %p, pGTABaseAddress = %p, szGTASOLibFile = %s, szExternalASIDir = %s)", pGTAHandle, pGTABaseAddress, szGTASOLibFile, szExternalASIDir);

    void *sym = dlsym(pGTAHandle, "_ZN5CRGBAC2Ehhhh");
    if (sym == NULL) {
        ALOGE("Symbol CRGBA::CRGBA is not exported.");
        return;
    }

    ALOGI("Symbol CRGBA::CRGBA is located at %p.", sym);

    if ((unsigned int)(sym) & 1) {
        // thumb mode (WarDrum)
        Redirect(sym, CRGBA__ctor_thumb, 1);
    }
    else {
        // arm mode (Lucid)
        Redirect(sym, CRGBA__ctor_arm, 0);
    }    
}
