#ifndef __LYW_CODE_TASK_POOL_HPP__
#define __LYW_CODE_TASK_POOL_HPP__

#include <pthread.h>

#include "TaskFunc.hpp"


namespace LYW_CODE
{
    class TaskPool 
    {
    private:
        typedef struct _Thread 
        {
            pthread_t pth;
        } Thread_t;

    private:
        int m_threadNum;

    public:
        TaskPool(int threadNum)
        {
            m_threadNum = threadNum;

        }

        int Start()
        {
            return 0;
        }

        int Stop()
        {
            return 0;
        }

        int AddTask()
        {
            return 0;
        }

        int RemoveTask()
        {
            return 0;
        }

    };
}
#endif
