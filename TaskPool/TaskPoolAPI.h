#ifndef __RM_CBB_API_TASK_POOL_API_FILE_H__
#define __RM_CBB_API_TASK_POOL_API_FILE_H__

#include <stdio.h>
#include "SimpleFunction.hpp"
#include "TaskPoolDefine.h"
#include "type.h"

typedef RM_CODE::Function3 <void(void *, xint32_t, void * )> TaskPoolCB_t;
typedef RM_CODE::Function4 <void(TaskPoolErrCode_e, void *, xint32_t, void * )> TaskPoolErrCB_t;

extern "C" {
    xint32_t RM_CBB_TaskPoolCreate(xint32_t maxTaskNum, xint32_t maxThreadNum);

    void RM_CBB_TaskPoolDestroy();
    
    void * RM_CBB_RegisterTimeTask(TaskPoolCB_t cb, TaskPoolErrCB_t errCB, void * userParam, xuint32_t interval,  bool IsSer = true, xuint32_t maxWaitTaskCount = 5);

    void * RM_CBB_RegisterNormalTask(TaskPoolCB_t cb, TaskPoolErrCB_t errCB, void * userParam, bool IsSer = true, xuint32_t maxWaitTaskCount = 5);
    
    xint32_t RM_CBB_UnRegisterTask(void * handle);
    
    xint32_t RM_CBB_AddTask(void * handle, void * param, xint32_t lenOfParam);

    void testX();
}
#endif
