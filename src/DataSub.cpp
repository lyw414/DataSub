#include <stdio.h>

namespace LYW_CODE
{
    extern "C" int LYW_CODE_SUB_IPC_SET(const char * id, unsigned int dataBufferCacheSize)
    {
        return 0;
    }

    extern "C" int LYW_CODE_SUB_SVR_TASK_SET(unsigned int maxThreadNum)
    {
        return 0;
    }
}
