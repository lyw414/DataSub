#include "TaskPoolAPI.h"
#include "TaskPool.hpp"

static RM_CODE::TaskPool * gTaskPool = new RM_CODE::TaskPool();

INTERFACE_API void RM_CBB_TaskPoolCreate(xint32_t maxTaskNum, xint32_t maxThreadNum)
{
    gTaskPool->Init(maxTaskNum, maxThreadNum);
    return;
}

INTERFACE_API void RM_CBB_TaskPoolDestroy()
{
    gTaskPool->UnInit();
    return;
}

/**
 * @brief           更新全局资源 - fork之后替换原有资源
 *
 */
INTERFACE_API void RM_CBB_TaskPoolNew()
{
    gTaskPool = new RM_CODE::TaskPool();
    return;
}
    

INTERFACE_API TaskHandle RM_CBB_RegisterTimeTask(TaskPoolCB_t cb, TaskPoolErrCB_t errCB, void * userParam, xuint32_t interval,  bool IsSer, xuint32_t maxWaitTaskCount)
{
    return gTaskPool->RegisterTimeTask(cb, errCB, userParam, interval, IsSer, maxWaitTaskCount );
}


INTERFACE_API TaskHandle RM_CBB_RegisterNormalTask(TaskPoolCB_t cb, TaskPoolErrCB_t errCB, void * userParam, bool IsSer, xuint32_t maxWaitTaskCount)
{
    return gTaskPool->RegisterNormalTask(cb, errCB,userParam, IsSer, maxWaitTaskCount);
}
    

INTERFACE_API xint32_t RM_CBB_UnRegisterTask(TaskHandle handle)

{
    return gTaskPool->UnRegisterTask(handle);
}
    

INTERFACE_API xint32_t RM_CBB_AddTask(TaskHandle handle, void * param, xint32_t lenOfParam)
{
    return gTaskPool->AsyncExecTask(handle, param, lenOfParam);
}

