#include "TaskPoolAPI.h"
#include "TaskPool.hpp"

static RM_CODE::TaskPool * g_taskPool = new RM_CODE::TaskPool();

xint32_t RM_CBB_TaskPoolCreate(xint32_t maxTaskNum, xint32_t maxThreadNum)
{
    g_taskPool->Init(maxTaskNum, maxThreadNum);
}

void RM_CBB_TaskPoolDestroy()
{
    g_taskPool->UnInit();
}
    
void * RM_CBB_RegisterTimeTask(TaskPoolCB_t cb, TaskPoolErrCB_t errCB, void * userParam, xuint32_t interval,  bool isSer, xuint32_t maxWaitTaskCount)
{
    return g_taskPool->RegisterTimeTask(cb, errCB, userParam, interval, isSer, maxWaitTaskCount );
}

void * RM_CBB_RegisterNormalTask(TaskPoolCB_t cb, TaskPoolErrCB_t errCB, void * userParam, bool isSer, xuint32_t maxWaitTaskCount)
{
    return g_taskPool->RegisterNormalTask(cb, errCB,userParam, isSer, maxWaitTaskCount);
}
    
xint32_t RM_CBB_UnRegisterTask(void * handle)
{
    return g_taskPool->UnRegisterTask(handle);
}
    
xint32_t RM_CBB_AddTask(void * handle, void * param, xint32_t lenOfParam)
{
    return g_taskPool->AsyncExecTask(handle, param, lenOfParam);
}

void testX()
{
    //g_taskPool->AsyncExecTask(handle, param, lenOfParam);
}

