#ifndef __RM_CBB_API_TASK_POOL_API_FILE_H__
#define __RM_CBB_API_TASK_POOL_API_FILE_H__

#include <stdio.h>
#include "SimpleFunction.hpp"
#include "TaskPoolDefine.h"
#include "streamaxcomdev.h"

typedef RM_CODE::Function3 <void(void *, xint32_t, void * )> TaskPoolCB_t;
typedef RM_CODE::Function4 <void(TaskPoolErrCode_e, void *, xint32_t, void * )> TaskPoolErrCB_t;

typedef void * TaskHandle;

extern "C" {
    INTERFACE_API void RM_CBB_TaskPoolCreate(xint32_t maxTaskNum, xint32_t maxThreadNum);

    INTERFACE_API void RM_CBB_TaskPoolDestroy();

    INTERFACE_API void RM_CBB_TaskPoolNew();
    
    INTERFACE_API TaskHandle RM_CBB_RegisterTimeTask(TaskPoolCB_t cb, TaskPoolErrCB_t errCB, void * userParam, xuint32_t interval,  bool IsSer = true, xuint32_t maxWaitTaskCount = 5);

    INTERFACE_API TaskHandle RM_CBB_RegisterNormalTask(TaskPoolCB_t cb, TaskPoolErrCB_t errCB, void * userParam, bool IsSer = true, xuint32_t maxWaitTaskCount = 5);
    
    INTERFACE_API xint32_t RM_CBB_UnRegisterTask(TaskHandle handle);
    
    INTERFACE_API xint32_t RM_CBB_AddTask(TaskHandle handle, void * param, xint32_t lenOfParam);
}
#endif
