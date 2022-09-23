#ifndef __RM_CODE_TASK_POOL_FILE_HPP__
#define __RM_CODE_TASK_POOL_FILE_HPP__

#include "SimpleFunction.hpp"
#include "FixLenList.hpp"

#include <map>
#include <string.h>
#include <pthread.h>
#include <vector>
#include <stdio.h>
#include <unistd.h>

namespace RM_CODE
{
    class TaskPool 
    {
    private:
        typedef enum _TaskST {
            TASK_ST_DOING,      //执行中
            TASK_ST_WAIT,       //等待
            TASK_ST_FREE,       //无任务执行
        } TaskST_e;

        typedef enum _ThreadST {
            THREAD_ST_RUN,       //工作
            THREAD_ST_KILL,      //终止中 线程执行完成任务后退出
            THREAD_ST_FINISHED,  //终止 可以回收线程资源
        } ThreadST_e;

        typedef enum _ThreadType {
            THREAD_TYPE_TIME,    //定时任务线程
            THREAD_TYPE_MON,     //独占任务线程
            THREAD_TYPE_MNG,     //管理线程管理线程
            THREAD_TYPE_WORK,    //工作线程
        } ThreadType_e;

        typedef enum _TaskMode {
            TASK_MODE_MON,          //独占
            TASK_MODE_ASYNC,        //异步 - 允许并
            TASK_MODE_SYNC,         //同步 - 任务排队
            TASK_MODE_TIME_ASYNC,   //定时异步 - 允许并发
            TASK_MODE_TIME_SYNC     //定时同步 - 任务排队
        } TaskMode_e;

    public:
        typedef Function3 <void(void *, xint32_t, void *)> CallBack_t;

    private:
        struct _TaskQueueNode;

    public:
        typedef struct _TaskAttr {
            //0 阻塞 超时时间 ms
            xint32_t timeout;
            //调度间隔 精度10ms
            xint64_t interval;
            xint32_t maxWaitNum;
            TaskMode_e mode;
            xint32_t waitCount;
            FixLenList<struct _TaskQueueNode *> * waitQueue; 
        } TaskAttr_t;

        typedef struct _Task {
            xint32_t ID;
            CallBack_t handleFunc;
            CallBack_t errFunc;
            void * userParam;
            TaskAttr_t attr;
        } Task_t;

    private:
        typedef struct _TaskQueueNode {
            Task_t * task;
            TaskST_e st; //任务状态
            xuint32_t taskID;
            FixLenList<struct _TaskQueueNode *>::ListNode_t * waitQueueNode; //同步任务等待列表节点位置 
            FixLenList<struct _TaskQueueNode *>::ListNode_t * taskQueueNode; //列表节点指针
            struct timespec startTime; //开始时间
            void * param;
            xint32_t lenOfParam;
        } TaskQueueNode_t;

        typedef struct _ThreadInfo {
            ::pthread_t handle;
            xuint32_t  threadID;;
            struct timespec startTime; //任务开始时间
            ThreadST_e st;
            ThreadType_e type;
            struct _TaskQueueNode * taskNode; //任务
        } ThreadInfo_t;

        typedef struct _ThreadParam {
            TaskPool * self;
            ThreadInfo_t * threadInfo;
        } ThreadParam_t;

        typedef struct _MonThreadParam {
            TaskPool * self;
            Task_t * task;
        } MonThreadParam_t;



    private:
        //注册任务
        std::map<xuint32_t, Task_t *> m_unRegisterTask; //管理线程清理
        std::map<xuint32_t, Task_t *> m_registerTask;  //注册 注销
        xint32_t m_registerTaskVer; //更新计数器 - 版本



        //定时任务
        std::map<xuint32_t, Task_t *> m_timeTask; //注册 注销 管理
        xint32_t m_timeTaskVer; //更新计数器 - 版本
        
        //独占任务
        std::map<xuint32_t, bool> m_monTask;
        
        ::pthread_mutex_t m_taskLock;

        xuint32_t m_taskIDRecord;

        //任务队列
        FixLenList<TaskQueueNode_t *> m_taskQueue;
        
        //空闲任务节点
        FixLenList<TaskQueueNode_t *> m_freeTaskNode;
        
        //任务节点资源
        TaskQueueNode_t * m_taskQueueNode;

        //变长支持 
        std::vector<ThreadInfo_t *> m_threadNode;

        xuint32_t m_threadIDRecord;
        
    private:
        //0 结束 1 运行 2 等待结束中
        xint32_t m_st;

        xuint32_t m_maxThreadNum;

        xuint32_t m_maxTaskNum;

        ::pthread_mutex_t m_lock;

    private:
        xint32_t CreateThread(ThreadType_e type, void* (* enter)(void *), TaskQueueNode_t * taskNode = NULL)
        {
            ThreadInfo_t * pThreadInfo = NULL;
            ThreadParam_t * pThreadParam = NULL;
            pthread_t handle;
            pthread_attr_t attr;


            pThreadParam = new ThreadParam_t;
            pThreadInfo = new ThreadInfo_t;
                
            pThreadParam->self = this;
            pThreadParam->threadInfo = pThreadInfo;

            pThreadInfo->type = type;
            pThreadInfo->handle = handle;
            pThreadInfo->startTime.tv_sec = 0;
            pThreadInfo->startTime.tv_nsec = 0;

            pThreadInfo->threadID = m_threadIDRecord++;

            pThreadInfo->st = THREAD_ST_RUN;
            pThreadInfo->taskNode = taskNode;

            m_threadNode.push_back(pThreadInfo);

            ::pthread_attr_init(&attr); 
            ::pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

            ::pthread_create(&handle, NULL, enter, pThreadParam);

            return 0;
        }

        xint32_t CreateMonThread(Task_t * task)
        {


        }


        static void * WorkThreadDo_ (void * ptr)
        {
            ThreadParam_t * param = (ThreadParam_t *)ptr;
            if (param != NULL)
            {
                param->self->WorkThreadDo(param);
                param->threadInfo->st = THREAD_ST_FINISHED;
                delete param;
            }
            return NULL;
        }


        void WorkThreadDo(ThreadParam_t * param)
        {
            TaskQueueNode_t * taskNode;
            TaskQueueNode_t * waitQueueTaskNode;
            while (param->threadInfo->st ==  THREAD_ST_RUN)
            {
                if (!m_taskQueue.Empty())
                {
                    ::pthread_mutex_lock(&m_taskLock);
                    if (m_taskQueue.Front(taskNode) != NULL)
                    {
                        m_taskQueue.Pop_front();
                        taskNode->taskQueueNode = NULL;
                    }
                    else
                    {
                        taskNode = NULL;
                    }
                    ::pthread_mutex_unlock(&m_taskLock);
                }

                if (taskNode == NULL)
                {
                    usleep(10000);
                    continue;
                }


                taskNode->task->handleFunc(taskNode->param, taskNode->lenOfParam, taskNode->task->userParam);

                ::pthread_mutex_lock(&m_taskLock);
                if (taskNode->task->attr.mode == TASK_MODE_SYNC || taskNode->task->attr.mode == TASK_MODE_TIME_SYNC)
                {
                    //同步任务 需要将等待队列的任务添加至执行队列
                    if (taskNode->task->attr.waitQueue != NULL)
                    {
                        taskNode->task->attr.waitCount--;
                        if (!taskNode->task->attr.waitQueue->Empty())
                        {
                            taskNode->task->attr.waitQueue->Front(waitQueueTaskNode);
                            taskNode->task->attr.waitQueue->Pop_front();
                            waitQueueTaskNode->waitQueueNode = NULL;
                            waitQueueTaskNode->taskQueueNode = m_taskQueue.Push_back(waitQueueTaskNode);
                        }
                    }
                }
                m_freeTaskNode.Push_back(taskNode);
                ::pthread_mutex_unlock(&m_taskLock);

                taskNode = NULL;
            }
        }

        static void * TimeThreadDo_ (void * ptr)
        {
            ThreadParam_t * param = (ThreadParam_t *)ptr;
            if (param != NULL)
            {
                param->self->TimeThreadDo(param);
                param->threadInfo->st = THREAD_ST_FINISHED;
                delete param;
            }
            return NULL;

        }

        void TimeThreadDo(ThreadParam_t * param)
        {
            xint32_t ver = 0;
            std::map<xuint32_t, Task_t *>::iterator it;
            std::map<xuint32_t, Task_t *> timeTask; 
            std::vector<std::pair<struct timespec, Task_t *> > array;
            std::vector<std::pair<struct timespec, Task_t *> > tmp;
            struct timespec now; 

            xint64_t interval = 0;

            ::pthread_mutex_unlock(&m_lock);
            ver = m_timeTaskVer;
            timeTask = m_timeTask;
            ::pthread_mutex_lock(&m_lock);

            for (xint32_t iLoop = 0; iLoop < array.size(); iLoop++)
            {
                if (timeTask.find(array[iLoop].second->ID) != timeTask.end())
                {
                    tmp.push_back(array[iLoop]);
                }
                else
                {
                    timeTask.erase(array[iLoop].second->ID);
                }
            }

            array.clear();
            array = tmp;
            tmp.clear();

            for(it = m_timeTask.begin(); it != m_timeTask.end(); it++)
            {
                ::clock_gettime(CLOCK_MONOTONIC, &now);
                array.push_back(std::pair<struct timespec, Task_t *>(now, it->second));
            }

            while (param->threadInfo->st == THREAD_ST_RUN)
            {
                if (ver != m_timeTaskVer)
                {
                    ::pthread_mutex_unlock(&m_lock);
                    ver = m_timeTaskVer;
                    timeTask = m_timeTask;
                    ::pthread_mutex_lock(&m_lock);

                    for (xint32_t iLoop = 0; iLoop < array.size(); iLoop++)
                    {
                        if (timeTask.find(array[iLoop].second->ID) != timeTask.end())
                        {
                            tmp.push_back(array[iLoop]);
                        }
                        else
                        {
                            timeTask.erase(array[iLoop].second->ID);
                        }
                    }
                    array.clear();
                    array = tmp;
                    tmp.clear();
                    
                    for(it = m_timeTask.begin(); it != m_timeTask.end(); it++)
                    {
                        ::clock_gettime(CLOCK_MONOTONIC, &now);
                        array.push_back(std::pair<struct timespec, Task_t *>(now, it->second));
                    }
                }

                ::clock_gettime(CLOCK_MONOTONIC, &now);

                for (xint32_t iLoop = 0; iLoop < array.size(); iLoop++)
                {
                    interval = (now.tv_sec - array[iLoop].first.tv_sec) * 1000 + (now.tv_nsec - array[iLoop].first.tv_nsec) / 1000000;

                    //printf("ZZZZZZZZZZZZZZZ %ld %ld\n", array[iLoop].first.tv_sec, array[iLoop].first.tv_nsec);

                    //printf("ZZZZZZZZZZZZZZZ1 %ld %ld\n", now.tv_sec, now.tv_nsec);
                    //printf("ZZZZZZZZZZZZZZZ2 %lld\n", interval);

                    if (interval > array[iLoop].second->attr.interval)
                    {
                        array[iLoop].first = now;
                        AddTask(array[iLoop].second, NULL, 0);
                        //printf("SSSSSSSSS %d\n", array[iLoop].second->ID);

                    }
                }
                ::usleep(10000);
            }
        }


        static void * MonThreadDo_ (void * ptr)
        {
            ThreadParam_t * param = (ThreadParam_t *)ptr;
            if (param != NULL)
            {
                param->self->MonThreadDo(param);
                param->threadInfo->st = THREAD_ST_FINISHED;
                delete param;
            }
            return NULL;
        }

        void MonThreadDo(ThreadParam_t * param)
        {
            while (param->threadInfo->st ==  THREAD_ST_RUN)
            {
                //printf("Mon Runing......\n");
                sleep(10);
            }
        }

        static void * MngThreadDo_ (void * ptr)
        {
            ThreadParam_t * param = (ThreadParam_t *)ptr;
            if (param != NULL)
            {
                param->self->MngThreadDo(param);
                delete param->threadInfo;
                delete param;
            }
            
            return NULL;
        }

        void MngThreadDo(ThreadParam_t * param)
        {
            while (m_st == 1)
            {
                //printf("Mng Runing......\n");
                sleep(10);
            }
            
            //设置自己的状态
            param->threadInfo->st = THREAD_ST_FINISHED;
            pthread_mutex_lock(&m_lock);
            ClearResource();
            pthread_mutex_unlock(&m_lock);

            //printf("Clear End!\n");
            //完成清理 
            m_st = 0;
        }

        //仅允许管理线程使用 
        void ClearResource()
        {
            //设置线程状态
            for (xint32_t iLoop = 0; iLoop < m_threadNode.size(); iLoop++)
            {
                if (m_threadNode[iLoop] != NULL && m_threadNode[iLoop]->st != THREAD_ST_FINISHED)
                {
                    m_threadNode[iLoop]->st = THREAD_ST_KILL;
                }
            }

            //等待线程结束
            for (xint32_t iLoop = 0; iLoop < m_threadNode.size(); iLoop++)
            {
                while(m_threadNode[iLoop]->st != THREAD_ST_FINISHED)
                {
                    usleep(10000);
                }
            }

            m_threadNode.clear();

            //清理申请的任务节点
            delete m_taskQueueNode;

            m_taskQueueNode = NULL;

            m_freeTaskNode.UnInit();

            m_taskQueue.UnInit();

            //清理注册的任务资源
            std::map<xuint32_t, Task_t *>::iterator it;

            for (it = m_registerTask.begin(); it != m_registerTask.end(); it++)
            {
                if (it->second != NULL)
                {
                    if (it->second->attr.waitQueue != NULL)
                    {
                        it->second->attr.waitQueue->UnInit();
                        delete it->second->attr.waitQueue;
                    }

                    delete it->second;
                }
            }

            m_registerTask.clear();
            m_timeTask.clear();
            m_monTask.clear();
        }

        void ClearTheadNode (ThreadInfo_t * threadInfo)
        {
            for (xint32_t iLoop = 0; iLoop < m_threadNode.size(); iLoop++)
            {
                if (m_threadNode[iLoop] != NULL && m_threadNode[iLoop]->threadID == threadInfo->threadID)
                {
                    //数组可以在此处删除
                    m_threadNode[iLoop] == NULL;
                    delete threadInfo;
                    break;
                }
            }
        }

    public:
        TaskPool(xuint32_t maxTaskNum, xuint32_t maxThreadNum) : m_taskQueue(maxTaskNum), m_freeTaskNode(maxTaskNum)
        {
            m_maxTaskNum = maxTaskNum;

            m_maxThreadNum = maxThreadNum;

            m_registerTaskVer = 0;

            m_timeTaskVer = 0;

            m_st = 0;

            m_threadIDRecord = 0;

            m_taskIDRecord = 0;

            ::pthread_mutex_init(&m_lock, NULL);

            ::pthread_mutex_init(&m_taskLock, NULL);
        }

        ~TaskPool()
        {
            m_taskQueue.UnInit();
        }


        xint32_t Init() 
        {
            ::pthread_mutex_lock(&m_lock);
            if (m_st == 0)
            {
                m_st = 1;

                m_taskQueue.Init();

                m_freeTaskNode.Init();

                m_taskQueueNode = new TaskQueueNode_t[m_maxTaskNum];
                for (xint32_t iLoop = 0; iLoop < m_maxTaskNum; iLoop++)
                {
                    m_taskQueueNode[iLoop].task = NULL;
                    m_taskQueueNode[iLoop].st = TASK_ST_FREE;
                    m_taskQueueNode[iLoop].taskID = 0;
                    m_taskQueueNode[iLoop].waitQueueNode = NULL;
                    m_taskQueueNode[iLoop].taskQueueNode = NULL;
                    m_taskQueueNode[iLoop].startTime.tv_sec = 0;
                    m_taskQueueNode[iLoop].startTime.tv_nsec = 0;
                    m_taskQueueNode[iLoop].param = NULL;
                    m_taskQueueNode[iLoop].lenOfParam = 0;

                    m_freeTaskNode.Push_back(&m_taskQueueNode[iLoop]);
                }

                //管理线程
                CreateThread(THREAD_TYPE_MNG, MngThreadDo_);

                //定时任务线程
                CreateThread(THREAD_TYPE_TIME, TimeThreadDo_);

                //任务线程 
                for (xint32_t iLoop = 0; iLoop < m_maxThreadNum; iLoop++)
                {
                    CreateThread(THREAD_TYPE_WORK, WorkThreadDo_);
                }

                ::pthread_mutex_unlock(&m_lock);

                return 0;
            }
            else
            {
                ::pthread_mutex_unlock(&m_lock);
                return -1;
            }
        }

        xint32_t UnInit() 
        {
            xint32_t retry = 10;
            ::pthread_mutex_lock(&m_lock);
            if (m_st == 1)
            {
                //管理线程清理资源
                m_st = 2;
                
                ::pthread_mutex_unlock(&m_lock);

                while (m_st != 0 && retry > 0)
                {
                    usleep(500000);
                    retry--;
                }
                return 0;
            }
            else
            {
                ::pthread_mutex_unlock(&m_lock);
                return -1;
            }

        }

        void SetMonTaskAttr(TaskAttr_t * attr)
        {
            return;
        }

        void SetNormalTaskAttr(TaskAttr_t * attr, bool isSync, xuint32_t maxWaitNum, xuint32_t timeout)
        {
            if (attr == NULL)
            {
                return;
            }

            if (isSync == true)
            {
                attr->mode = TASK_MODE_SYNC;
                attr->timeout = timeout;
                attr->interval = 0;
                attr->maxWaitNum = maxWaitNum;
                attr->waitCount = 0;
            }
            else
            {
                attr->mode = TASK_MODE_ASYNC;
                attr->timeout = timeout;
                attr->interval = 0;
                attr->maxWaitNum = 0;
                attr->waitCount = 0;
            }
        }

        void SetTimeTaskAttr(TaskAttr_t * attr, xuint32_t interval,  bool isSync, xuint32_t maxWaitNum, xuint32_t timeout)
        {
            if (attr == NULL)
            {
                return;
            }

            if (isSync == true)
            {
                attr->mode = TASK_MODE_TIME_SYNC;
                attr->timeout = timeout;
                attr->interval = interval;
                attr->maxWaitNum = maxWaitNum;
                attr->waitCount = 0;
            }
            else
            {
                attr->mode = TASK_MODE_TIME_ASYNC;
                attr->timeout = timeout;
                attr->interval = interval;
                attr->maxWaitNum = 0;
                attr->waitCount = 0;
            }

        }

        Task_t * RegisterTask(CallBack_t handleFunc, void * userParam, const TaskAttr_t * attr = NULL, CallBack_t errFunc = NULL)
        {
            Task_t * task = new Task_t;

            TaskQueueNode_t * taskNode = NULL;


            task->handleFunc = handleFunc;
            task->errFunc = errFunc;
            task->userParam = userParam;
            if (attr != NULL)
            {
                task->attr = *attr;
            }
            else
            {
                SetNormalTaskAttr(&task->attr, false, 0, 0);
            }

            ::pthread_mutex_lock(&m_lock);

            do 
            {
                task->ID = m_taskIDRecord++;
            } while(m_registerTask.find(task->ID) != m_registerTask.end());

            m_registerTask[task->ID] = task;

            m_registerTaskVer++;

            switch(task->attr.mode) 
            {
            case TASK_MODE_TIME_SYNC:
            case TASK_MODE_SYNC:
                task->attr.waitQueue = new FixLenList<struct _TaskQueueNode *> (task->attr.maxWaitNum); 
                task->attr.waitQueue->Init();
                break;
            default:
                break;
            }

            switch(task->attr.mode) 
            {
            case TASK_MODE_TIME_ASYNC:
            case TASK_MODE_TIME_SYNC:
                m_timeTask[task->ID] = task;
                m_timeTaskVer++;
                break;
            case TASK_MODE_MON:
                m_monTask[task->ID] = false;
                CreateThread(THREAD_TYPE_MON, MonThreadDo_);
                break;
            }
            ::pthread_mutex_unlock(&m_lock);
            return task;
        }

        void UnRegisterTask(Task_t * task)
        {
            ::pthread_mutex_lock(&m_lock);

            if (m_registerTask.find(task->ID) != m_registerTask.end() )
            {
                m_registerTaskVer++;
                m_unRegisterTask[task->ID] = task;
                switch (task->attr.mode)
                {
                case TASK_MODE_TIME_ASYNC:
                case TASK_MODE_TIME_SYNC:
                    m_registerTask.erase(task->ID);
                    m_timeTask.erase(task->ID);
                    m_timeTaskVer++;
                    break;
                case THREAD_TYPE_MON:
                    //独占线程 需要找到对应线程 并结束
                    for (xuint32_t iLoop = 0; iLoop < m_threadNode.size(); iLoop++)
                    {

                        //独占任务 会长固定占有一个任务节点 所以任务节点里的任务资源 与 当前资源匹配 则说明为当前任务资源的独占线程
                        if (m_threadNode[iLoop]->taskNode != NULL && m_threadNode[iLoop]->taskNode->task->ID == task->ID)
                        {
                            //独占任务 终止即可 由管理线程检测线程结束后 清理即可
                            m_threadNode[iLoop]->st = THREAD_ST_KILL;
                            break;
                        }
                    }
                    break;
                }
            }
            ::pthread_mutex_unlock(&m_lock);
        }

        xint32_t AddTask(Task_t * task, void * param, int lenOfParam)
        {
            TaskQueueNode_t * taskNode = NULL;
            TaskQueueNode_t * waitQueueTaskNode = NULL;
            FixLenList<struct _TaskQueueNode *>::ListNode_t * listNode = NULL;
            //FixLenList<struct _TaskQueueNode *>::ListNode_t * waitQueueNode;
            ::pthread_mutex_lock(&m_taskLock);
            if (!m_freeTaskNode.Empty())
            {
                m_freeTaskNode.Front(taskNode);
                m_freeTaskNode.Pop_front();

                taskNode->task = task;
                taskNode->st = TASK_ST_WAIT;
                taskNode->param = param;
                taskNode->lenOfParam = lenOfParam;

                ::clock_gettime(CLOCK_MONOTONIC, &taskNode->startTime);
                taskNode->taskID = m_taskIDRecord++;

                if (task->attr.mode == TASK_MODE_SYNC || task->attr.mode == TASK_MODE_TIME_SYNC)
                {
                    //存在任务处于执行状态
                    if (task->attr.waitQueue != NULL && task->attr.waitCount > 0)
                    {
                        if (task->attr.waitQueue->Full())
                        {
                            //任务已满 移除最旧的排队任务
                            task->attr.waitQueue->Front(waitQueueTaskNode);
                            task->attr.waitQueue->Pop_front();
                            waitQueueTaskNode->st = TASK_ST_FREE;
                            listNode = m_freeTaskNode.Push_back(waitQueueTaskNode);
                        }
                        else
                        {
                            task->attr.waitCount++;
                        }
                        
                        //排队
                        task->attr.waitQueue->Push_back(taskNode);
                        taskNode->waitQueueNode = listNode;
                        taskNode->taskQueueNode = NULL;
                    }
                    else
                    {
                        //无任务排队 直接加入执行任务队列 
                        listNode = m_taskQueue.Push_back(taskNode);
                        task->attr.waitCount++;
                        taskNode->taskQueueNode = listNode;
                        taskNode->waitQueueNode = NULL;
                    }
                }
                else
                {
                    listNode = m_taskQueue.Push_back(taskNode);
                    taskNode->taskQueueNode = listNode;
                    taskNode->waitQueueNode = NULL;
                }
                ::pthread_mutex_unlock(&m_taskLock);
                
                //通知

                return 0;
            }
            else
            {
                ::pthread_mutex_unlock(&m_taskLock);
            }
            return -1;
        }
    };
}
#endif
