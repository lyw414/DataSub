#ifndef __RM_CODE_TASK_POOL_FILE_HPP__
#define __RM_CODE_TASK_POOL_FILE_HPP__

#include <pthread.h>
#include <vector>
#include <map>
#include <list>
#include <unistd.h>

#include <stdio.h>

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
        typedef RM_CODE::Function4 <void(TaskPoolErrCode_e errCode, void *, xint32_t, void * )> ErrCallBack;

        typedef enum _TaskPoolST {
            TASK_POOL_ST_START,
            TASK_POOL_ST_STOPPING,
            TASK_POOL_ST_STOPPED
        } TaskPoolST_e;

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
            TASK_ST_FINISHED
        } TaskST_e;
        
        /**
         * @brief   对于串行任务需要关注任务资源状态 
         *
         */
        typedef enum _TaskResourceST {
            TASK_RESOURCE_ST_USE,           //可用
            TASK_RESOURCE_ST_FREEING,       //释放中 任务资源被定时任务持有时 设置为该状态 
            TASK_RESOURCE_ST_FREED          //释放完成
        } TaskResourceST_e;

        typedef enum _TaskResourceType {
            TASK_RESOURCE_TYPE_MON,          //独占任务
            TASK_RESOURCE_TYPE_SER,          //串行任务
            TASK_RESOURCE_TYPE_CONC,         //并行任务
            TASK_RESOURCE_TYPE_TIME_SER,     //定时串行任务
            TASK_RESOURCE_TYPE_TIME_CONC     //定时并行任务
        } TaskResourceType_e;

    private:
        struct _TaskNode;

        typedef struct _TaskAttr {
            xuint32_t interval;
            xuint32_t timeout;
            xuint32_t maxWaitTaskCount;
            xuint32_t waitCount;
            FixLenList<struct _TaskNode *> * waitTaskNode;
        } TaskAttr_t;

        typedef struct _TaskResource {
            xint32_t taskResourceID;                //任务资源ID
            CallBack handleFunc;                    //处理函数
            ErrCallBack errFunc;                       //异常函数
            void * userParam;                       //用户参数指针
            TaskResourceType_e taskResourceType;    //任务资源类型
            TaskResourceST_e taskResourceST;        //任务资源状态

            TaskAttr_t attr;
        } TaskResource_t;

        typedef struct _TimeTask {
            xint64_t leftTime;
            TaskResource_t * taskResource;
        } TimeTask_t;

        typedef struct _TaskNode {
            TaskResource_t * taskResource;
            TaskST_e taskST;                                                    //任务状态 
            TaskPoolErrCode_e errCode;                                          //错误码
            xuint32_t taskID;                                                   //任务ID 任务唯一标识
            void * param;                                                       //参数指针 - 资源由用户维护
            xint32_t lenOfParam;                                                //参数长度 - 参数长度
            struct timespec startTime;                                          //任务开始时间
            FixLenList<struct _TaskNode *>::ListNode_t * taskQueueListNode;       //用于在m_taskQueue中快速移除任务节点
            FixLenList<struct _TaskNode *>::ListNode_t * waitQueueListNode;       //用于在TaskResource_t::attr::waitQueue 中快速移除任务节点
        } TaskNode_t;


        typedef struct _ThreadInfo {
            pthread_t handle;           //线程句柄
            xint32_t  threadID;         //线程ID 唯一标识
            ThreadST_e threadST;        //线程状态
            ThreadType_e threadType;    //线程类型
            TaskNode_t * taskNode;      //当前处理的任务节点资源
            struct timespec aliveTime;  //最后一次心跳时间
        } ThreadInfo_t;

        typedef struct _ThreadParam {
            TaskPool * self;
            ThreadInfo_t * threadInfo;
        } ThreadParam_t;

    private:
        pthread_mutex_t m_lock;
        xuint32_t m_maxTaskCount;
        xuint32_t m_maxThreadCount;
        TaskPoolST_e m_taskPoolST;

        //任务资源
        std::map<xint32_t, TaskResource_t *> m_taskResource;
        std::map<xint32_t, TaskResource_t *> m_waitFreeTaskResource;
        xuint32_t  m_waitFreeTaskResourceVer;

        std::map<xint32_t, TaskResource_t *> m_timeTaskResource;
        xuint32_t m_timeTaskResourceVer;
        std::map<xint32_t, TaskResource_t *> m_monTaskResource;
        
        //任务
        xint32_t m_taskSize;
        TaskNode_t * m_totalTask;
        FixLenList<TaskNode_t *> m_taskQueue;
        FixLenList<TaskNode_t *> m_freeQueue;
        pthread_mutex_t m_taskLock;

        pthread_mutex_t m_taskStartCondLock;
        pthread_cond_t m_taskStartCond;
        pthread_condattr_t m_taskStartCondAttr;

        pthread_mutex_t m_taskFinishedCondLock;
        pthread_cond_t m_taskFinishedCond;
        pthread_condattr_t m_taskFinishedCondAttr;

        //线程
        std::vector<ThreadInfo_t *> m_threadInfo; 
        xuint32_t m_threadInfoVer;
        
    private:

        void FreeAllResource(xint32_t timeout = 5)
        {
            xint32_t tag = 0;

            std::map<xint32_t, TaskResource_t *>::iterator it;
            ::pthread_mutex_lock(&m_lock);
            for (xint32_t iLoop = 0; iLoop < m_threadInfo.size(); iLoop++ )
            {
                if (m_threadInfo[iLoop] != NULL && m_threadInfo[iLoop]->threadST == THREAD_ST_RUN)
                {
                    m_threadInfo[iLoop]->threadST = THREAD_ST_KILL;
                }
            }
            ::pthread_mutex_unlock(&m_lock);
            
            while(timeout > 0)
            {
                tag = 1;
                for (xint32_t iLoop = 0; iLoop < m_threadInfo.size(); iLoop++ )
                {
                    if (m_threadInfo[iLoop] != NULL)
                    {
                        if ( m_threadInfo[iLoop]->threadST != THREAD_ST_FINISHED)
                        {
                           tag = 0;
                        }
                        else
                        {
                            delete m_threadInfo[iLoop];
                            m_threadInfo[iLoop] = NULL;
                        }
                    }
                }

                if (tag == 1)
                {
                    break;
                }

                ::sleep(1);
                timeout--;
            }

            for (it = m_taskResource.begin(); it != m_taskResource.end(); it++)
            {
                if (it->second->attr.waitTaskNode != NULL)
                {
                    it->second->attr.waitTaskNode->UnInit();
                    delete it->second->attr.waitTaskNode;
                }
                delete it->second;
            }

            for (it = m_waitFreeTaskResource.begin(); it != m_waitFreeTaskResource.end(); it++)
            {
                if (it->second->attr.waitTaskNode != NULL)
                {
                    it->second->attr.waitTaskNode->UnInit();
                    delete it->second->attr.waitTaskNode;
                }
                delete it->second;
            }

            m_taskQueue.UnInit();
            m_freeQueue.UnInit();
            delete m_totalTask;
        }

        xuint32_t GenTaskID()
        {
            static xuint32_t id = 0;
            return (id++);
        }

        xint32_t GenTaskResourceID()
        {
            static xint32_t id = 0;
            if ((id++) < 0)
            {
                id = 0;
            }
            return id;
        }
        
        xint32_t CheckAndUploadThreadInfo(xuint32_t & ver, std::vector<ThreadInfo_t *> & thread)
        {
            if (ver != m_threadInfoVer)
            {
                ::pthread_mutex_lock(&m_lock);
                ver = m_threadInfoVer;
                thread = m_threadInfo;
                ::pthread_mutex_unlock(&m_lock);
            }

            return 0;
        }

        xint32_t CheckAndUploadFreeTaskResource(xuint32_t & ver, std::list<TaskResource_t *> & freeTaskResource)
        {
            std::map<xint32_t, TaskResource_t *>::iterator it;
            
            if (ver != m_waitFreeTaskResourceVer)
            {
                ::pthread_mutex_lock(&m_lock);
                ver = m_waitFreeTaskResourceVer;
                for (it = m_waitFreeTaskResource.begin(); it != m_waitFreeTaskResource.end(); it++)
                {
                    freeTaskResource.push_back(it->second);
                }
                m_waitFreeTaskResource.clear();
                ::pthread_mutex_unlock(&m_lock);
            }
        }



        xint32_t CheckAndUploadTimeTaskResource(xuint32_t & ver, std::vector<TimeTask_t> & taskResource, xuint32_t & minInterval)
        {
            TimeTask_t timeTask = {0};
            std::vector<TimeTask_t> tmp;
            std::map<xint32_t, TaskResource_t *> timeResourceMap;
            std::map<xint32_t, TaskResource_t *>::iterator it;


            if (ver != m_timeTaskResourceVer)
            {
                ::pthread_mutex_lock(&m_lock);
                ver = m_timeTaskResourceVer;
                timeResourceMap = m_timeTaskResource;
                ::pthread_mutex_unlock(&m_lock);
                minInterval = 1000000;
                
                for (xint32_t iLoop = 0; iLoop < taskResource.size(); iLoop++)
                {
                    if (timeResourceMap.find(taskResource[iLoop].taskResource->taskResourceID) != timeResourceMap.end())
                    {
                        //update
                        taskResource[iLoop].taskResource = timeResourceMap[taskResource[iLoop].taskResource->taskResourceID];
                        tmp.push_back(taskResource[iLoop]);
                        timeResourceMap.erase(taskResource[iLoop].taskResource->taskResourceID);
                        if (minInterval > taskResource[iLoop].taskResource->attr.interval * 1000)
                        {
                            minInterval = taskResource[iLoop].taskResource->attr.interval * 1000;
                        }
                    }
                    else
                    {
                        taskResource[iLoop].taskResource->taskResourceST = TASK_RESOURCE_ST_FREED;
                    }
                }

                for (it = timeResourceMap.begin(); it !=  timeResourceMap.end(); it++)
                {
                    timeTask.leftTime = 0;
                    timeTask.taskResource = it->second;
                    tmp.push_back(timeTask);
                    if (minInterval > it->second->attr.interval * 1000)
                    {
                        minInterval = it->second->attr.interval * 1000;
                    }
                }

                taskResource = tmp;

                return minInterval;
            }
            else
            {
                return minInterval;
            }
        }

        xint32_t CreateThread(ThreadType_e type, TaskResource_t * taskResource = NULL)
        {
            ThreadParam_t * threadParam = new ThreadParam_t;
            ThreadInfo_t * threadInfo = new ThreadInfo_t;
            pthread_attr_t attr;
            
            threadParam->self = this;
            threadParam->threadInfo = threadInfo;

            threadInfo->threadID = GenTaskResourceID();
            threadInfo->threadST = THREAD_ST_RUN;
            threadInfo->threadType = type;

            threadInfo->taskNode = NULL;

            m_threadInfo.push_back(threadInfo);
            m_threadInfoVer++;

            switch(type)
            {
            case THREAD_TYPE_TIME:
            {
                ::pthread_attr_init(&attr); 
                ::pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
                ::pthread_create(&threadInfo->handle, &attr, TimeThreadEnter, threadParam);
                break;
            }
            case THREAD_TYPE_MON:
            {
                TaskNode_t * taskNode = NULL;
                xint32_t retry = 5;

                ::pthread_attr_init(&attr); 
                ::pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
                if (taskResource == NULL)
                {
                    return -1;
                }

                while(retry > 0)
                {
                    retry--;
                    ::pthread_mutex_lock(&m_taskLock);
                    if (m_freeQueue.Empty())    
                    {
                        ::pthread_mutex_unlock(&m_taskLock);
                        if (retry <= 0)
                        {
                            return -2;
                        }
                        ::usleep(500000);
                    }
                    else
                    {
                        m_freeQueue.Front(taskNode);
                        m_freeQueue.Pop_front();
                        taskNode->taskID = GenTaskID();
                        ::pthread_mutex_unlock(&m_taskLock);
                        break;
                    }
                }

                taskNode->taskResource = taskResource;

                ::pthread_create(&threadInfo->handle, &attr, MonThreadEnter, threadParam);
                break;
            }
            case THREAD_TYPE_MNG:
            {
                ::pthread_attr_init(&attr); 
                ::pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
                ::pthread_create(&threadInfo->handle, &attr, MngThreadEnter, threadParam);
                break;
            }
            case THREAD_TYPE_WORK:
            {
                ::pthread_attr_init(&attr); 
                ::pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
                ::pthread_create(&threadInfo->handle, &attr, WorkThreadEnter, threadParam);
                break;
            }
            default:
                break;
            }

            return 0;
        }

        static void * TimeThreadEnter(void *ptr)
        {
            ThreadParam_t * threadParam = (ThreadParam_t *)ptr;
            threadParam->self->TimeThreadRun(threadParam->threadInfo);
            delete threadParam;
            return NULL;
        }

        void TimeThreadRun(ThreadInfo_t * threadInfo)
        {
            std::vector<TimeTask_t> timeTask;
            xuint32_t ver;
            xuint32_t interval = 0;
            xint64_t usedTime = 0;


            struct timespec nowTime;
            struct timespec lastTime;

            ::clock_gettime(CLOCK_MONOTONIC, &nowTime);
            ::clock_gettime(CLOCK_MONOTONIC, &lastTime);

            ver = m_timeTaskResourceVer - 100;

            while (threadInfo->threadST == THREAD_ST_RUN)
            {
                ::clock_gettime(CLOCK_MONOTONIC, &nowTime);
                CheckAndUploadTimeTaskResource(ver, timeTask, interval);
                usedTime = (nowTime.tv_sec - lastTime.tv_sec) * 1000 +  (nowTime.tv_nsec - lastTime.tv_nsec) / 1000000;
                if (usedTime > 0)
                {
                    lastTime = nowTime;
                }

                for(xint32_t iLoop = 0; iLoop < timeTask.size(); iLoop++)
                {
                    timeTask[iLoop].leftTime -= usedTime;
                    if (timeTask[iLoop].leftTime <= 0)
                    {
                        timeTask[iLoop].leftTime = timeTask[iLoop].taskResource->attr.interval;
                        AsyncExecTask(timeTask[iLoop].taskResource, NULL, 0);
                        //printf("ssss %d\n", timeTask[iLoop].taskResource->attr.interval);
                    }
                }
                ::usleep(interval);
            }

            threadInfo->threadST = THREAD_ST_FINISHED;
            printf("FINISHED\n");
        }

        static void * MngThreadEnter(void *ptr)
        {
            ThreadParam_t * threadParam = (ThreadParam_t *)ptr;
            threadParam->self->MngThreadRun(threadParam->threadInfo);
            delete threadParam;
            return NULL;
        }

        void MngThreadRun(ThreadInfo_t * threadInfo)
        {
            xint32_t interval = 1000000;
            xint64_t leftTime = 0;

            std::list<TaskResource_t *> freeTaskResource;
            std::list<TaskResource_t *>::iterator it;
            xuint32_t freeTaskResourceVer = m_waitFreeTaskResourceVer - 100;

            std::vector<ThreadInfo_t *> thread; 
            xuint32_t threadInfoVer = m_threadInfoVer - 100;
            xint32_t threadCheckInterval = 0;

            while(m_taskPoolST == TASK_POOL_ST_START)
            {
                //检测注销任务资源 确认无任务执行后释放资源
                CheckAndUploadFreeTaskResource(freeTaskResourceVer, freeTaskResource);
                for (it = freeTaskResource.begin(); it != freeTaskResource.end();) 
                {
                    if ((*it)->taskResourceST == TASK_RESOURCE_ST_FREED && (*it)->attr.waitTaskNode == 0)
                    {
                        if ((*it)->attr.waitTaskNode != NULL) 
                        {
                            (*it)->attr.waitTaskNode->UnInit();
                            delete (*it)->attr.waitTaskNode;
                        }
                        delete *it;
                        it = freeTaskResource.erase(it);
                    }
                    else
                    {
                        it++;
                    }
                }

                //线程资源管理
                if (threadCheckInterval <= 0)
                {
                    threadCheckInterval = 2000000;
                    CheckAndUploadThreadInfo(threadInfoVer, thread);
                    for (xint32_t iLoop = 0; iLoop < thread.size(); iLoop++)
                    {
                        if (thread[iLoop]->threadST == THREAD_ST_FINISHED)
                        {
                            ::pthread_mutex_lock(&m_lock);
                            for (xint32_t i = 0; i< m_threadInfo.size(); i++)
                            {
                                if (m_threadInfo[i] == thread[iLoop])
                                {
                                    delete m_threadInfo[i];
                                    m_threadInfo[i] = NULL;
                                    m_threadInfoVer++;
                                    break;
                                }
                            }
                            ::pthread_mutex_unlock(&m_lock);
                        }
                    }
                }
                else
                {
                    threadCheckInterval -= interval;
                }

                ::usleep(interval);
            }

            threadInfo->threadST = THREAD_ST_FINISHED;

            FreeAllResource();

            
            //释放待清理任务资源缓存中的内容
            for (it = freeTaskResource.begin(); it != freeTaskResource.end();) 
            {
                if ((*it)->attr.waitTaskNode != NULL) 
                {
                    (*it)->attr.waitTaskNode->UnInit();
                    delete (*it)->attr.waitTaskNode;
                }
                delete *it;
            }

            m_taskPoolST = TASK_POOL_ST_STOPPED;

            printf("FINISHED\n");
        }

        static void * MonThreadEnter(void *ptr)
        {
            ThreadParam_t * threadParam = (ThreadParam_t *)ptr;
            threadParam->self->MonThreadRun(threadParam->threadInfo);
            delete threadParam;
            return NULL;
        }

        void MonThreadRun(ThreadInfo_t * threadInfo)
        {
            if (threadInfo->taskNode == NULL)
            {
                return;
            }

            while(threadInfo->threadST == THREAD_ST_RUN)
            {
                sleep(1);
            }
            
            ::pthread_mutex_lock(&m_taskLock);
            m_freeQueue.Push_back(threadInfo->taskNode);
            ::pthread_mutex_unlock(&m_taskLock);

            threadInfo->threadST = THREAD_ST_FINISHED;
        }

        static void * WorkThreadEnter(void *ptr)
        {
            ThreadParam_t * threadParam = (ThreadParam_t *)ptr;
            threadParam->self->WorkThreadRun(threadParam->threadInfo);
            delete threadParam;
            return NULL;
        }

        void WorkThreadRun(ThreadInfo_t * threadInfo)
        {
            TaskNode_t * taskNode = NULL;
            TaskNode_t * tmp = NULL;

            struct timespec wakeTime;
            while(threadInfo->threadST == THREAD_ST_RUN || !m_taskQueue.Empty())
            {
                taskNode = NULL;

                ::clock_gettime(CLOCK_MONOTONIC, &(threadInfo->aliveTime));
                if (!m_taskQueue.Empty())
                {
                    ::pthread_mutex_lock(&m_taskLock);
                    if (!m_taskQueue.Empty())
                    {
                        m_taskQueue.Front(taskNode);
                        m_taskQueue.Pop_front();
                        taskNode->taskST = TASK_ST_DOING;
                    }
                    ::pthread_mutex_unlock(&m_taskLock);
                }

                if (taskNode == NULL)
                {
                    ::clock_gettime(CLOCK_MONOTONIC, & wakeTime);
                    wakeTime.tv_sec += 2;
                    ::pthread_mutex_lock(&m_taskStartCondLock);
                    ::pthread_cond_timedwait(&m_taskStartCond, &m_taskStartCondLock, &wakeTime);
                    ::pthread_mutex_unlock(&m_taskStartCondLock);
                }
                else
                {
                    if(taskNode->taskResource->taskResourceST == TASK_RESOURCE_ST_USE)
                    {
                        if (taskNode->errCode == TASK_POOL_NORMAL)
                        {
                            //任务节点正常 执行正常处理逻辑
                            if (taskNode->taskResource->handleFunc != NULL)
                            {
                                taskNode->taskResource->handleFunc(taskNode->param, taskNode->lenOfParam, taskNode->taskResource->userParam);
                            }
                        }
                        else
                        {
                            //任务节点发生异常 执行异常处理逻辑
                            if (taskNode->taskResource->errFunc != NULL)
                            {
                                taskNode->taskResource->errFunc(taskNode->errCode, taskNode->param, taskNode->lenOfParam, taskNode->taskResource->userParam);
                            }
                        }
                    }

                    taskNode->taskST = TASK_ST_FINISHED;
                    taskNode->errCode = TASK_POOL_NORMAL;
                    
                    ::pthread_mutex_lock(&m_taskLock);
                    taskNode->taskResource->attr.waitCount--;
                    

                    if (taskNode->taskResource->attr.waitTaskNode != NULL && !(taskNode->taskResource->attr.waitTaskNode->Empty()))
                    {
                        taskNode->taskResource->attr.waitTaskNode->Front(tmp);
                        taskNode->taskResource->attr.waitTaskNode->Pop_front();
                        tmp->waitQueueListNode = NULL;

                        tmp->taskQueueListNode = m_taskQueue.Push_back(tmp);
                    }
                    taskNode->taskQueueListNode = NULL;
                    taskNode->waitQueueListNode = NULL;
                    m_freeQueue.Push_back(taskNode);
                    ::pthread_mutex_unlock(&m_taskLock);
                }
            }
            
            threadInfo->threadST = THREAD_ST_FINISHED;

            printf("FINISHED\n");
        }

    public:
        TaskPool()
        {
            ::pthread_mutex_init(&m_lock, NULL);
            ::pthread_mutex_init(&m_taskLock, NULL);
            ::pthread_mutex_init(&m_taskStartCondLock, NULL);
            ::pthread_condattr_setclock(&m_taskStartCondAttr, CLOCK_MONOTONIC);
            ::pthread_cond_init(&m_taskStartCond, &m_taskStartCondAttr);

            ::pthread_mutex_init(&m_taskFinishedCondLock, NULL);
            ::pthread_condattr_setclock(&m_taskFinishedCondAttr, CLOCK_MONOTONIC);
            ::pthread_cond_init(&m_taskFinishedCond, &m_taskFinishedCondAttr);


            m_maxTaskCount = 0;
            m_maxThreadCount = 0;
            m_taskPoolST = TASK_POOL_ST_STOPPED;
            m_taskSize = 0;

            m_timeTaskResourceVer = 0;
            m_threadInfoVer = 0;
            m_waitFreeTaskResourceVer = 0;

            m_totalTask = NULL;

        }

        xint32_t Init(xint32_t maxTaskCount, xint32_t maxThreadCount)
        {
            ::pthread_mutex_lock(&m_lock);
            if (m_taskPoolST == TASK_POOL_ST_STOPPED)
            {
                m_taskPoolST = TASK_POOL_ST_START;

                m_maxTaskCount = maxTaskCount;
                m_maxThreadCount = maxThreadCount;
                
                m_totalTask = new TaskNode_t[m_maxTaskCount];
                m_taskQueue.Init(m_maxTaskCount);
                m_freeQueue.Init(m_maxTaskCount);

                for(xint32_t iLoop = 0; iLoop < m_maxTaskCount; iLoop++)
                {
                    m_totalTask[iLoop].taskResource = NULL;
                    m_totalTask[iLoop].taskST = TASK_ST_FINISHED;
                    m_totalTask[iLoop].errCode = TASK_POOL_NORMAL;
                    m_totalTask[iLoop].taskID = 0;
                    m_totalTask[iLoop].param = NULL;
                    m_totalTask[iLoop].lenOfParam = 0;
                    m_totalTask[iLoop].startTime.tv_sec = 0;
                    m_totalTask[iLoop].startTime.tv_nsec = 0;
                    m_totalTask[iLoop].taskQueueListNode = NULL;
                    m_totalTask[iLoop].waitQueueListNode = NULL;

                    m_freeQueue.Push_back(&m_totalTask[iLoop]);
                }


                CreateThread(THREAD_TYPE_MNG);

                CreateThread(THREAD_TYPE_TIME);

                for (xint32_t iLoop = 0; iLoop < m_maxThreadCount; iLoop++)
                {
                    CreateThread(THREAD_TYPE_WORK);
                }
                ::pthread_mutex_unlock(&m_lock);
                return 0;
            }
            else
            {
                ::pthread_mutex_unlock(&m_lock);
            }
            return -1;
        }

        void UnInit()
        {
            ::pthread_mutex_lock(&m_lock);
            if (m_taskPoolST == TASK_POOL_ST_START)
            {
                m_taskPoolST = TASK_POOL_ST_STOPPING;
            }
            ::pthread_mutex_unlock(&m_lock);
        }

        void * RegisterNormalTask(CallBack handleFunc, ErrCallBack errFunc, void * userParam, bool IsSer = true, xuint32_t maxWaitTaskCount = 5)
        {
            if (m_taskPoolST != TASK_POOL_ST_START)
            {
                return NULL;
            }

            TaskResource_t * taskResource = new TaskResource_t;

            xint32_t  resourceID = 0;
            ::pthread_mutex_lock(&m_lock);
            resourceID = GenTaskResourceID();
            m_taskResource[resourceID] = taskResource;

            taskResource->taskResourceID = resourceID;
            taskResource->handleFunc = handleFunc;
            taskResource->errFunc = errFunc;
            taskResource->userParam = userParam;
            taskResource->taskResourceST = TASK_RESOURCE_ST_USE;

            if (IsSer)
            {
                taskResource->taskResourceType = TASK_RESOURCE_TYPE_SER;
                taskResource->attr.timeout = 0;
                taskResource->attr.maxWaitTaskCount = maxWaitTaskCount + 1;
                taskResource->attr.waitCount = 0;
                taskResource->attr.waitTaskNode = new FixLenList<struct _TaskNode *>;

                taskResource->attr.waitTaskNode->Init(taskResource->attr.maxWaitTaskCount);

            }
            else
            {
                taskResource->taskResourceType = TASK_RESOURCE_TYPE_CONC;
                taskResource->attr.timeout = 0;
                taskResource->attr.maxWaitTaskCount = 0;
                taskResource->attr.waitCount = 0;
                taskResource->attr.waitTaskNode = NULL;
            }
            ::pthread_mutex_unlock(&m_lock);
            return taskResource;
        }

        void * RegisterTimeTask(CallBack handleFunc, ErrCallBack errFunc, void * userParam, xuint32_t interval, bool IsSer = true, xuint32_t maxWaitTaskCount = 5)
        {
            if (m_taskPoolST != TASK_POOL_ST_START)
            {
                return NULL;
            }

            TaskResource_t * taskResource = new TaskResource_t;

            xint32_t resourceID = 0;
            ::pthread_mutex_lock(&m_lock);
            m_timeTaskResourceVer++;
            resourceID = GenTaskResourceID();
            m_taskResource[resourceID] = taskResource;

            taskResource->taskResourceID = resourceID;
            taskResource->handleFunc = handleFunc;
            taskResource->errFunc = errFunc;
            taskResource->userParam = userParam;
            taskResource->taskResourceST = TASK_RESOURCE_ST_USE;

            if (IsSer)
            {
                taskResource->taskResourceType = TASK_RESOURCE_TYPE_TIME_SER;
                taskResource->attr.timeout = 0;
                taskResource->attr.interval = interval;
                taskResource->attr.maxWaitTaskCount = maxWaitTaskCount + 1;
                taskResource->attr.waitCount = 0;
                taskResource->attr.waitTaskNode = new FixLenList<struct _TaskNode *>;

                taskResource->attr.waitTaskNode->Init(maxWaitTaskCount);
            }
            else
            {
                taskResource->taskResourceType = TASK_RESOURCE_TYPE_TIME_CONC;
                taskResource->attr.timeout = 0;
                taskResource->attr.interval = interval;
                taskResource->attr.maxWaitTaskCount = 0;
                taskResource->attr.waitCount = 0;
                taskResource->attr.waitTaskNode = NULL;

            }
            m_timeTaskResource[resourceID] = taskResource;
            ::pthread_mutex_unlock(&m_lock);
            return taskResource;
        }

        void * RegisterMonTask(CallBack handleFunc, ErrCallBack errFunc, void * userParam)
        {
            if (m_taskPoolST != TASK_POOL_ST_START)
            {
                return NULL;
            }

            TaskResource_t * taskResource = new TaskResource_t;
            xint32_t  resourceID = 0;
            ::pthread_mutex_lock(&m_lock);
            resourceID = GenTaskResourceID();
            m_taskResource[resourceID] = taskResource;

            taskResource->taskResourceID = resourceID;
            taskResource->handleFunc = handleFunc;
            taskResource->errFunc = errFunc;
            taskResource->userParam = userParam;
            taskResource->taskResourceST = TASK_RESOURCE_ST_USE;
            taskResource->taskResourceType = TASK_RESOURCE_TYPE_MON;
            taskResource->attr.interval = 0;
            taskResource->attr.maxWaitTaskCount = 0;
            taskResource->attr.waitCount = 0;
            taskResource->attr.waitTaskNode = NULL;

            m_monTaskResource[resourceID] = taskResource;
            ::pthread_mutex_unlock(&m_lock);

            return taskResource;
        }

        xint32_t UnRegisterTask(void * taskHandle)
        {
            TaskResource_t * taskResource = (TaskResource_t *)taskHandle;
            if (m_taskPoolST != TASK_POOL_ST_START)
            {
                return -1;
            }

            if (taskResource == NULL)
            {
                return -1;
            }

            pthread_mutex_lock(&m_lock);

            
            if (m_timeTaskResource.find(taskResource->taskResourceID) != m_timeTaskResource.end())
            {
                taskResource->taskResourceST = TASK_RESOURCE_ST_FREEING;
                m_timeTaskResource.erase(taskResource->taskResourceID);
                m_timeTaskResourceVer++;
            }

            if (m_taskResource.find(taskResource->taskResourceID) != m_taskResource.end())
            {
                if (taskResource->taskResourceST != TASK_RESOURCE_ST_FREEING)
                {
                    //资源没有被 定时任务 独占线程 可以等待所有任务完成后释放资源 - 管理线程检测
                    taskResource->taskResourceST = TASK_RESOURCE_ST_FREED;
                }
                m_taskResource.erase(taskResource->taskResourceID);
                m_waitFreeTaskResource[taskResource->taskResourceID] = taskResource;
            }
            pthread_mutex_unlock(&m_lock);
            return 0;
        }

        xint32_t AsyncExecTask(void * handle, void * param, xint32_t lenOfParam, xuint32_t * taskID = NULL)
        {
            TaskResource_t * taskResource = (TaskResource_t *)handle;
            TaskNode_t * taskNode = NULL;

            xint32_t timeout = 0;

            xint32_t isBlock = 0;

            xint32_t interval = 0;

            TaskNode_t * tmp = NULL;

            if (m_taskPoolST != TASK_POOL_ST_START)
            {
                return -1;
            }


            if (taskResource == NULL)
            {
                return -1;
            }

            timeout = taskResource->attr.timeout;

            if (timeout <= 0)
            {
                isBlock = 1;
            }


            while(true)
            {
                if (m_freeQueue.Empty())
                {
                    ::usleep(interval);
                    if (isBlock != 1)
                    {
                        timeout -= interval + 1;
                        if (timeout <= 0)
                        {
                            return -1;
                        }
                    }
                    continue;
                }

                ::pthread_mutex_lock(&m_taskLock);
                if (m_freeQueue.Empty())
                {
                    ::pthread_mutex_unlock(&m_taskLock);
                    continue;
                }
                else
                {
                    m_freeQueue.Front(taskNode);
                    m_freeQueue.Pop_front();
                    taskNode->taskResource = taskResource;
                    taskNode->taskST = TASK_ST_WAIT;
                    taskNode->errCode = TASK_POOL_NORMAL;
                    taskNode->taskID = GenTaskID();
                    taskNode->param = param;
                    taskNode->lenOfParam = lenOfParam;
                    ::clock_gettime(CLOCK_MONOTONIC, &(taskNode->startTime));
                    if (taskResource->taskResourceType == TASK_RESOURCE_TYPE_SER || taskResource->taskResourceType == TASK_RESOURCE_TYPE_TIME_SER)

                    {
                        if (taskResource->attr.waitCount != 0 && taskResource->attr.waitTaskNode != NULL)
                        {
                            taskResource->attr.waitCount++;
                            if (taskResource->attr.waitTaskNode->Full())
                            {
                                //将丢弃任务添加至执行队列中 进行报错处理
                                taskResource->attr.waitTaskNode->Front(tmp);
                                tmp->errCode = TASK_POOL_ERR_QUEUE_FULL;
                                m_taskQueue.Push_back(tmp);
                                taskResource->attr.waitTaskNode->Pop_front();
                                //taskResource->attr.waitCount--;
                            }

                            taskNode->waitQueueListNode = taskResource->attr.waitTaskNode->Push_back(taskNode);
                            taskNode->taskQueueListNode = NULL;
                        }
                        else
                        {
                            taskResource->attr.waitCount++;
                            taskNode->taskQueueListNode = m_taskQueue.Push_back(taskNode);
                            taskNode->waitQueueListNode = NULL;
                        }
                    }
                    else
                    {
                        taskResource->attr.waitCount++;
                        taskNode->taskQueueListNode = m_taskQueue.Push_back(taskNode);
                        taskNode->waitQueueListNode = NULL;
                    }
                }
                ::pthread_mutex_unlock(&m_taskLock);

                if (taskID != NULL)
                {
                    *taskID = taskNode->taskID;
                }

                ::pthread_cond_signal(&m_taskStartCond);

                
                //if (tmp != NULL)
                //{
                //    tmp->taskResource->errFunc(TASK_POOL_ERR_QUEUE_FULL, tmp->param, tmp->lenOfParam, tmp->taskResource->userParam);
                //    tmp->taskST = TASK_ST_FINISHED;
                //    tmp->errCode = TASK_POOL_NORMAL;
                //    ::pthread_mutex_lock(&m_taskLock);
                //    m_freeQueue.Push_back(tmp);
                //    ::pthread_mutex_unlock(&m_taskLock);
                //}

                return 0;
            }
        }

        xint32_t SyncHandleTask()
        {
            //等待任务完成
            return -1;
        }

    public:

    };
}
#endif
