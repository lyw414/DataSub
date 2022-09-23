#ifndef __RM_CODE_TASK_POOL_FILE_HPP__
#define __RM_CODE_TASK_POOL_FILE_HPP__

#include "SimpleFunction.hpp"
#include "TaskPoolDefine.h"
#include "FixLenList.hpp"
#include "type.h"

namespace RM_CODE
{
    class TaskPool 
    {
    private:
        typedef RM_CODE::Function3 <void(void *, xint32_t, void * )> CallBack;

        typedef enum _ThreadType {
            THREAD_TYPE_TIME,
            THREAD_TYPE_MON,
            THREAD_TYPE_MNG,
            THREAD_TYPE_WORK
        } ThreadType_e;

        typedef enum _ThreadST {
            THREAD_ST_KILL,
            THREAD_ST_RUN,
            THREAD_ST_FINISHED
        } ThreadST_e;

        typedef enum _TaskST {
            TASK_ST_WAIT,
            TASK_ST_DOING,
            TASK_ST_FREE
        } TaskST_e;

        typedef enum _TaskMode {
            TASK_MODE_MON,
            TASK_MODE_SYNC,
            TASK_MODE_ASYNC,
            TASK_MODE_TIME_SYNC,
            TASK_MODE_TIME_ASYNC
        } TaskMode_e;


    private:
        typedef struct _TaskResource {

        } TaskResource_t;


        typedef struct _TaskNode {

        } TaskNode_t;

        typedef struct _ThreadInfo {

        } ThreadInfo_t;

        typedef struct _ThreadParam {
            
        } ThreadParam_t;

    private:

    public:
        TaskPool()
        {
        }

        xint32_t Init(xint32_t maxTaskNum, xint32_t maxThreadNum)
        {
            return 0;
        }

        void UnInit()
        {
        }

        xint32_t RegisterTask( )
        {
            return 0;
        }


    public:

    };
}
#endif
