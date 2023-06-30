#include <stdlib.h>
#include "amvenc_common.h"

debug_log_level_t g_amvenc_log_level = ERR;

void amvenc_set_log_level(debug_log_level_t level)
{
    char *log_level = getenv("AMVENC_LOG_LEVEL");
    if (log_level) {
        g_amvenc_log_level = (debug_log_level_t)atoi(log_level);
        printf("Set log level by environment to %d\n", g_amvenc_log_level);
    } else {
        g_amvenc_log_level = level;
        printf("Set log level to %d\n", g_amvenc_log_level);
    }
    return;
}

