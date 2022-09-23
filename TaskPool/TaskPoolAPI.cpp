#include "TaskPoolAPI.h"
#include "TaskPool.hpp"

static RM_CODE::TaskPool * g_taskPool = new RM_CODE::TaskPool();

xint32_t RM_CBB_TaskPoolInit(xint32_t maxTaskNum, xint32_t maxThreadNum)
{
    g_taskPool->Init(maxTaskNum, maxThreadNum);
    return 0;
}

void RM_CBB_TaskPoolUnInit()
{
    g_taskPool->UnInit();
}

void RM_CBB_RegisterTask()
{
}

xint32_t RM_CBB_UnRegisterTask()
{
    return 0;
}

xint32_t RM_CBB_AddTask()
{
    return 0;
}


