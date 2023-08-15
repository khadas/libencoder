#ifndef __AML_VENC_INST_HELPER_H_
#define __AML_VENC_INST_HELPER_H_

#include <inttypes.h>
#include <string.h>
#include <algorithm>
#include <string>
#include <stdio.h>
#include "AmlVencParamIntf.h"

namespace android {

typedef enum _DumpFileType {
    DUMP_RAW,
    DUMP_ES
}eDumpFileType;


class AmlVencInstHelper {

public:
    AmlVencInstHelper();
    ~AmlVencInstHelper();

public:
    void SetCodecType(eCodecType Codec);
    void SetCodecHandle(long Handle);
    bool OpenFile(eDumpFileType type);
    uint32_t dumpDataToFile(eDumpFileType type,uint8_t *data,uint32_t size);
    void CloseFile(eDumpFileType type);

private:
    eCodecType mCodec;
    bool mDumpYuvEnable;
    bool mDumpEsEnable;
    int mFdDumpInput;
    int mFdDumpOutput;
    long mHandle;


};



}





#endif
