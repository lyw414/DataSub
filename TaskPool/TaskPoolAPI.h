#ifndef __RM_CBB_API_TASK_POOL_API_FILE_H__
#define __RM_CBB_API_TASK_POOL_API_FILE_H__

#include <stdio.h>
#include "SimpleFunction.hpp"
#include "TaskPoolDefine.h"
#include "type.h"

typedef RM_CODE::Function3 <void(void *, xint32_t, void * )> TaskPoolCB_t;

extern "C" {
    xint32_t RM_CBB_TaskPoolInit(xint32_t maxTaskNum = 0, xint32_t maxThreadNum = 0);
    
    void RM_CBB_SetTimeTaskAttr( );

    void RM_CBB_TaskPoolUnInit();
    
    void RM_CBB_RegisterTask();
    
    xint32_t RM_CBB_UnRegisterTask();
    
    xint32_t RM_CBB_AddTask();
}
#endif
