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


void Redirect(void *src, void *dst)
{
    // assume thumb mode
    unsigned int adjustedSrc = (unsigned int)(src) & 0xFFFFFFFE;
    unsigned int adjustedDst = (unsigned int)(dst) | 0x00000001;
    char code[8];

    *(unsigned int *)&code[0] = 0xF000F8DF;  // ldr.w pc, [pc] ; +0x04
    *(unsigned int *)&code[4] = adjustedDst; // .word DEST

    memcpy((void *)adjustedSrc, code, sizeof(code));
    cacheflush(adjustedSrc, adjustedSrc + sizeof(code), 0);
}

CRGBA *CRGBA__ctor(CRGBA *this, __attribute__((unused)) char r, __attribute__((unused)) char g, __attribute__((unused)) char b, char a)
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

    Redirect(sym, CRGBA__ctor);
}
