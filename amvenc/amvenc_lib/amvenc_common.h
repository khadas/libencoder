#ifndef AML_VENC_HAL_COMMON_H
#define AML_VENC_HAL_COMMON_H

#ifdef __cplusplus
    extern "C" {
#endif
#include <stdio.h>

typedef enum
{
    NONE=0, INFO, DEBUG, WARN, ERR, TRACE, MAX_LOG_LEVEL
} debug_log_level_t;

extern debug_log_level_t g_amvenc_log_level;

#ifdef __ANDROID__
#include <log/log.h>
#undef LOG_NDEBUG
#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "amvenc"

#define VLOG(level, x...) \
    do { \
        if (level >= g_amvenc_log_level) { \
            if (level == NONE) \
                ALOGV(x); \
            if (level == INFO) \
                ALOGI(x); \
            else if (level == DEBUG) \
                ALOGD(x); \
            else if (level == WARN) \
                ALOGW(x); \
            else if (level >= ERR) \
                ALOGE(x); \
        } \
    }while(0)
#else
#define VLOG(level, fmt , var...) \
    do { \
        if (level >= g_amvenc_log_level) { \
            printf("[%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##var);\
        } \
    }while(0)

#endif

void amvenc_set_log_level(debug_log_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* AML_VENC_HAL_COMMON_H */
