#ifndef __LYW_CODE_TASK_POOL_HPP__
#define __LYW_CODE_TASK_POOL_HPP__

#include <pthread.h>
#include "DataSubFunc.hpp"
#include <list>
#include <unistd.h>
#include <stdio.h>

namespace LYW_CODE
{
    class TaskPool 
    {

    public:
        typedef struct _TaskNode
        {
            void * param;
            unsigned int lenOfParam;
            Function2<void (void *, unsigned int)> handleFunc;
            Function2<void (void *, unsigned int)> errFunc;
        } TaskNode_t;


    private:
        typedef struct _Thread 
        {
            pthread_t pth;
        } Thread_t;

    private:
        int m_threadNum;
        std::list <TaskNode_t> m_list;

        //条件变量
        pthread_mutex_t m_condLock;
        pthread_condattr_t m_condAttr;
        pthread_cond_t m_cond;

        pthread_mutex_t m_lock;
        int m_tag;

    private:
        static void * HandleTask_(void * ptr)
        {
            TaskPool * self = (TaskPool *)ptr;

            
            if (self != NULL)
            {
                self->HandleTask();
            }

            return NULL;
        }


        void HandleTask()
        {
            TaskNode_t taskNode;
            struct timespec tv;

            int tag = 0;
            while (m_tag == 1)
            {

                ::pthread_mutex_lock(&m_lock);
                tag = 0;
                if (m_list.size() > 0) 
                {
                    taskNode = m_list.front();
                    m_list.pop_front();
                    tag = 1;
                }
                ::pthread_mutex_unlock(&m_lock);

                if (tag == 0)
                {
                    //阻塞收
                    if (0 != ::clock_gettime(CLOCK_MONOTONIC, &tv))   
                    {
                        ::usleep(50000);                              
                        continue;
                    }   
                    
                    tv.tv_sec += 2;                                   
                    pthread_mutex_lock(&m_condLock);                  
                    pthread_cond_timedwait(&m_cond, &m_condLock, &tv);
                    pthread_mutex_unlock(&m_condLock);
                    continue;
                }
                
                taskNode.handleFunc(taskNode.param, taskNode.lenOfParam);
            }
        }

    public:
        TaskPool(int threadNum)
        {
            m_threadNum = threadNum;

            ::pthread_mutex_init(&m_lock, NULL);
            ::pthread_condattr_setclock(&m_condAttr, CLOCK_MONOTONIC);
            ::pthread_cond_init(&m_cond, &m_condAttr);

            ::pthread_mutex_init(&m_condLock, NULL);

            m_tag = 0;

        }

        int Start()
        {
            pthread_t handle;
            pthread_attr_t attr;

            pthread_mutex_lock(&m_lock);
            if (m_tag == 0)
            {
                m_tag = 1;
                pthread_mutex_unlock(&m_lock);
                for (int iLoop = 0; iLoop < m_threadNum; iLoop++)
                {
                    ::pthread_create(&handle, NULL, TaskPool::HandleTask_, this);
                    ::pthread_detach(handle);
                }
            }
            else
            {
                pthread_mutex_unlock(&m_lock);
            }

            return 0;
        }

        int Stop()
        {
            m_tag = 0;
            return 0;
        }

        int AddTask(TaskNode_t & taskNode)
        {
            pthread_mutex_lock(&m_lock);
            m_list.push_back(taskNode);
            pthread_mutex_unlock(&m_lock);
            
            pthread_cond_signal(&m_cond);
            return 0;
        }

    };
}
#endif
