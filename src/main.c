#include <jni.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <dlfcn.h>
#include <link.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <android/log.h>


#define LOG_TAG "GTA:SA ASI Loader"

#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


int  g_iLoaderInited;
int  g_iLoadedMods;
char g_szExternalASIDir[PATH_MAX];
char g_szInternalASIDir[PATH_MAX];
void *g_pGTAHandle;
void *g_pGTABaseAddress;


int CopyFile(const char *source, const char *destination, int mode)
{
    static char buffer[1024 * 512]; // 0.5MB
    int fdsrc, fddst;
    ssize_t n;

    fdsrc = open(source, O_RDONLY);
    if (fdsrc < 0) {
        ALOGE("Can't open source file %s for copying, err: %s (%d).", source, strerror(errno), errno);
        return 0;
    }

    fddst = open(destination, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (fddst < 0) {
        close(fdsrc);
        ALOGE("Can't open destination file %s for copying, err: %s (%d).", destination, strerror(errno), errno);
        return 0;
    }

    while ((n = read(fdsrc, buffer, sizeof(buffer))) > 0) {
        write(fddst, buffer, n);
    }

    close(fdsrc);
    close(fddst);
    return 1;
}

int DlIterator(struct dl_phdr_info *info, __attribute__((unused)) size_t size, void *data)
{
    int j;
    const char *cb = (const char *)data;
    const char *base = (const char *)info->dlpi_addr;
    const ElfW(Phdr) *first_load = NULL;

    for (j = 0; j < info->dlpi_phnum; j++) {
        const ElfW(Phdr) *phdr = &info->dlpi_phdr[j];

        if (phdr->p_type == PT_LOAD) {
            const char *beg = base + phdr->p_vaddr;
            const char *end = beg + phdr->p_memsz;

            if (first_load == NULL) first_load = phdr;
            if (beg <= cb && cb < end) {
                g_pGTABaseAddress = (void *)(base + first_load->p_vaddr);

                if (mprotect(g_pGTABaseAddress, first_load->p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
                    ALOGW("Can't unprotect libGTASA.so, err: %s (%d)", strerror(errno), errno);
                }
                return 1;
            }
        }
    }
    return 0;
}

int StoreGTAHandleAndBaseAddress(const char *file)
{
    char *err;
    void *pfnJNIOnLoad;

    g_pGTAHandle = dlopen(file, RTLD_LAZY);
    if (!g_pGTAHandle) {
        ALOGE("Can't dlopen file %s, err: %s", file, dlerror());
        return 0;
    }

    dlerror();
    pfnJNIOnLoad = dlsym(g_pGTAHandle, "JNI_OnLoad");
    err = dlerror();

    if (err != NULL) {
        ALOGE("Can't find symbol JNI_OnLoad in file %s, err: %s", file, err);
        return 0;
    }

    if (pfnJNIOnLoad == NULL) {
        ALOGE("File %s doesn't export JNI_OnLoad.", file);
        return 0;
    }

    if (!dl_iterate_phdr(DlIterator, pfnJNIOnLoad)) {
        ALOGE("Can't find base address of %s", file);
        return 0;
    }

    ALOGI("Shared library %s (handle = %p) is loaded at: %p (JNI_OnLoad = %p)", file, g_pGTAHandle, g_pGTABaseAddress, pfnJNIOnLoad);
    return 1;
}

int LoadASI(const char *file)
{
    void *handle;
    char *err;
    void (*pfnInit)(void *, void *, const char *);

    handle = dlopen(file, RTLD_LAZY);
    if (!handle) {
        ALOGE("Can't dlopen file %s, err: %s", file, dlerror());
        return 0;
    }

    dlerror();
    pfnInit = dlsym(handle, "InitializeASI");

    if ((err = dlerror()) != NULL) {
        ALOGW("Can't find symbol InitializeASI in file %s, err: %s", file, err);
        return 1;
    }

    if (pfnInit == NULL) {
        ALOGW("File %s doesn't export InitializeASI.", file);
        return 1;
    }

    pfnInit(g_pGTAHandle, g_pGTABaseAddress, g_szExternalASIDir);
    return 1;
}

void LoadASIExecutor(const char *directory, const char *file)
{
    char srcpath[PATH_MAX], dstpath[PATH_MAX];

    sprintf(srcpath, "%s%s", directory, file);
    sprintf(dstpath, "%s%s", g_szInternalASIDir, file);

    if (!CopyFile(srcpath, dstpath, 0755)) {
        ALOGE("Can't install ASI file: %s", file);
        return;
    }

    if (!LoadASI(dstpath)) {
        ALOGE("Can't load ASI file: %s", file);
        return;
    }

    g_iLoadedMods++;
    ALOGI("Loaded ASI file: %s", file);
}

void RemoveFileExecutor(const char *directory, const char *file)
{
    char path[PATH_MAX];

    sprintf(path, "%s%s", directory, file);
    if (unlink(path) != 0) {
        ALOGW("Can't remove internal ASI file %s, err: %s (%d)", file, strerror(errno), errno);
        return;
    }

    ALOGI("Removed internal ASI file: %s", file);
}

const char *GetExtension(const char *file)
{
    const char *dot;

    dot = strrchr(file, '.');

    if (dot == NULL) return "";
    return dot+1;
}

void FindFilesAndExecute(const char *directory, const char *extension, void (*pfnExec)(const char *, const char *))
{
    DIR *d;
    struct dirent *ent;

    d = opendir(directory);
    if (!d) {
        ALOGI("Can't open directory %s for scanning.", directory);
        return;
    }

    while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        if (extension == NULL || strcmp(extension, GetExtension(ent->d_name)) == 0) {
            pfnExec(directory, ent->d_name);
        }
    }

    closedir(d);
}

int LoadASIMods(void)
{
    int err;

    err = mkdir(g_szExternalASIDir, 0755);
    if (err != 0 && errno != EEXIST) {
        ALOGE("Can't create external ASI directory %s, err: %s (%d)", g_szExternalASIDir, strerror(errno), errno);
        return 0;
    }

    err = mkdir(g_szInternalASIDir, 0755);
    if (err != 0 && errno != EEXIST) {
        ALOGE("Can't create internal ASI directory %s, err: %s (%d)", g_szInternalASIDir, strerror(errno), errno);
        return 0;
    }

    // remove all cached ASI files
    FindFilesAndExecute(g_szInternalASIDir, "asi", RemoveFileExecutor);

    // copy files to internal and load them
    FindFilesAndExecute(g_szExternalASIDir, "asi", LoadASIExecutor);

    return 1;
}

JNIEXPORT void JNICALL
Java_com_wardrumstudios_utils_WarMedia_SetupASILoader(JNIEnv *env, __attribute__((unused)) jobject thiz, jstring baseDir, jstring internalDir)
{
    const char *dir;

    if (g_iLoaderInited) {
        ALOGW("Ignoring duplicate call to SetupASILoader.");
        return;
    }
    g_iLoaderInited = 1;

    ALOGI("Loading ASI mods ...");

    dir = (*env)->GetStringUTFChars(env, baseDir, NULL);
    if (!dir) {
        ALOGE("Can't get base directory location.");
        return;
    }
    sprintf(g_szExternalASIDir, "%sASI/", dir);
    (*env)->ReleaseStringUTFChars(env, baseDir, dir);

    dir = (*env)->GetStringUTFChars(env, internalDir, NULL);
    if (!dir) {
        ALOGE("Can't get internal directory location.");
        return;
    }
    sprintf(g_szInternalASIDir, "%sASI/", dir);
    (*env)->ReleaseStringUTFChars(env, internalDir, dir);

    ALOGI("ExternalASIDir = %s", g_szExternalASIDir);
    ALOGI("InternalASIDir = %s", g_szInternalASIDir);

    StoreGTAHandleAndBaseAddress("libGTASA.so");
    LoadASIMods();

    ALOGI("Loaded %d ASI mods!", g_iLoadedMods);
}
