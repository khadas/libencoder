//#define LOG_NDEBUG 0
#define LOG_TAG "AMLVencHelper"
#include <utils/Log.h>
#include <cutils/properties.h>

#include"AmlVencInstHelper.h"


namespace android {

#define ENABLE_DUMP_ES        1
#define ENABLE_DUMP_RAW       (1 << 1)
#define ENCODER_PROP_DUMP_DATA        "debug.vendor.media.c2.venc.dump_data"

AmlVencInstHelper::AmlVencInstHelper()
                  :mDumpYuvEnable(false),
                   mDumpEsEnable(false),
                   mFdDumpInput(0),
                   mFdDumpOutput(0),
                   mHandle(0) {

}

AmlVencInstHelper::~AmlVencInstHelper() {

}

void AmlVencInstHelper::SetCodecType(eCodecType Codec) {
    mCodec = Codec;
}


void AmlVencInstHelper::SetCodecHandle(long Handle) {
    mHandle = Handle;
}


//todo: dump file everytime
bool AmlVencInstHelper::OpenFile(eDumpFileType type) {
    char strTmp[128];
    uint32_t isDumpFile = 0;

    memset(strTmp,0,sizeof(strTmp));
    isDumpFile = property_get_int32(ENCODER_PROP_DUMP_DATA, 0);
    if ((isDumpFile & ENABLE_DUMP_RAW) && (DUMP_RAW == type)) {
        mDumpYuvEnable = true;
        sprintf(strTmp,"/data/venc_dump_%lx.%s",mHandle,"yuv");
        ALOGE("Enable Dump yuv file, name: %s", strTmp);
        mFdDumpInput = open(strTmp, O_CREAT | O_LARGEFILE | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
        if (mFdDumpInput < 0) {
            ALOGE("Dump Input File handle error!");
        }
    }

    if ((isDumpFile & ENABLE_DUMP_ES) && (DUMP_ES == type)) {
        mDumpEsEnable = true;
        sprintf(strTmp,"/data/venc_dump_%lx.%s",mHandle,((mCodec == H264) ? "h264" : "h265"));
        ALOGE("Enable Dump es file, name: %s", strTmp);
        mFdDumpOutput = open(strTmp, O_CREAT | O_LARGEFILE | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
        if (mFdDumpOutput < 0) {
            ALOGE("Dump Output File handle error!");
        }
    }
    return true;
}


uint32_t AmlVencInstHelper::dumpDataToFile(eDumpFileType type,uint8_t *data,uint32_t size) {
    uint32_t uWriteLen = 0;
    if (!((DUMP_RAW == type && mDumpYuvEnable) || (DUMP_ES == type && mDumpEsEnable))) {
        ALOGD("dump data not enable!");
        return 0;
    }

    int fd = (DUMP_RAW == type) ? mFdDumpInput : mFdDumpOutput;
    if (fd >= 0 && (data != 0)) {
        ALOGE("dump %s data size: %d",((DUMP_RAW == type) ? "raw" : "es"),size);
        uWriteLen = write(fd, (unsigned char *)data, size);
    }
    return uWriteLen;
}


void AmlVencInstHelper::CloseFile(eDumpFileType type) {
    if (mFdDumpInput && (DUMP_RAW == type)) {
        close(mFdDumpInput);
        mFdDumpInput = 0;
    }
    if (mFdDumpOutput && (DUMP_ES == type)) {
        close(mFdDumpOutput);
        mFdDumpOutput = 0;
    }
}



}



