#ifndef ANDROID_CODECUTILS_H
#define ANDROID_CODECUTILS_H

#include "VulkanAndroid.h"
#include <media/NdkMediaCodec.h>

static constexpr int COLOR_FormatSurface = 0x7F000789;

const char *amErrorString(media_status_t status);

#define AM_CHECK_RESULT(ctx, f)                                                                    \
{                                                                                                \
    media_status_t res = (f);                                                                    \
    if (res != AMEDIA_OK)                                                                        \
    {                                                                                            \
        LOGE("Fatal : %s \"%s\" in %s at line %d", ctx, amErrorString(res), __FILE__, __LINE__);\
    } else {                                                                                    \
        LOGI("OK : %s \"%s\" in %s at line %d", ctx, amErrorString(res), __FILE__, __LINE__);   \
    }                                                                                           \
}

#define AM_CHECK_RESULT_ERR(ctx, f)                                                                \
{                                                                                                \
    media_status_t res = (f);                                                                    \
    if (res != AMEDIA_OK)                                                                        \
    {                                                                                            \
        LOGE("Fatal : %s \"%s\" in %s at line %d", ctx, amErrorString(res), __FILE__, __LINE__);\
    }                                                                                           \
}

#endif //ANDROID_CODECUTILS_H
