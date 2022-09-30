#ifndef __RM_CODE_TASK_POOL_DEFINE_FILE_H__
#define __RM_CODE_TASK_POOL_DEFINE_FILE_H__
typedef enum _TaskPoolErrCode {
    TASK_POOL_NORMAL,
    TASK_POOL_ERR_TIMEOUT,
    TASK_POOL_ERR_QUEUE_FULL
} TaskPoolErrCode_e;
#endif
