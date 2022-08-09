#ifndef __LYW_CODE_TASK_POOL_HPP__
#define __LYW_CODE_TASK_POOL_HPP__

#include <pthread.h>

#include "DataSubFunc.hpp"

#include <list>

namespace LYW_CODE
{
    class TaskPool 
    {

    public:
        typedef struct _TaskNode
        {
            void * param;
            unsigned int lenOfParam;
            Fuction2<void (void *, unsigned int)> handleFunc;
        } TaskNode_t;


    private:
        typedef struct _Thread 
        {
            pthread_t pth;
        } Thread_t;

    private:
        int m_threadNum;

        std::list <TaskNode_t> m_waitingTaskList;

        std::list <TaskNode_t> m_doingTaskList;

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

        int AddTask(TaskNode_t & taskNode)
        {
            //m_waitingTaskList.push_back(taskNode);
            return 0;
        }

        int RemoveTask()
        {
            return 0;
        }

    };
}
#endif
