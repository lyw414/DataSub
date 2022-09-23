#ifndef __RM_CBB_API_TASK_POOL_API_FILE_H__
#define __RM_CBB_API_TASK_POOL_API_FILE_H__

#include <stdio.h>
#include "SimpleFunction.hpp"
#include "type.h"

extern "C" {
    xint32_t RM_CBB_TaskPoolInit(xint32_t maxTaskNum, xint32_t maxThreadNum);
    
    void RM_CBB_TaskPoolUnInit();
    
    void RM_CBB_RegisterTask();
    
    xint32_t RM_CBB_UnRegisterTask();
    
    xint32_t RM_CBB_AddTask();
}
#endif
