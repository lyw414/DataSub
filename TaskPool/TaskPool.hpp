#ifndef __RM_CODE_TASK_POOL_FILE_HPP__
#define __RM_CODE_TASK_POOL_FILE_HPP__

#include <pthread.h>
#include <vector>
#include <map>
#include <list>
#include <unistd.h>

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
            TASK_ST_FINISHED,
            TASK_ST_TIMEOUT
        } TaskST_e;
        
        /**
         * @brief   对于串行任务需要关注任务资源状态 
         *
         */
        typedef enum _TaskResourceST {
            TASK_RESOURCE_ST_OCCUPY,    //资源忙
            TASK_RESOURCE_ST_FREE,      //资源空闲
            TASK_RESOURCE_ST_DROP       
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

        typedef struct _TaskAttrTimeConc {
            xuint32_t interval;
            xuint32_t timeout;
            xint32_t leftTime;
        } TaskAttrTimeConc_t;

        typedef struct _TaskAttrTimeSer {
            xuint32_t interval;
            xuint32_t timeout;
            xint32_t leftTime;
            xuint32_t maxWaitTaskCount;
            xuint32_t waitCount;
            FixLenList<struct _TaskNode *> * waitTaskNode;
        } TaskAttrTimeSer_t;

        typedef struct _TaskAttrSer {
            xint32_t timeout;
            xuint32_t maxWaitTaskCount;
            xuint32_t waitCount;
            FixLenList<struct _TaskNode *> * waitTaskNode;
        } TaskAttrSer_t;

        typedef struct _TaskAttrConc {
            xuint32_t timeout;
        } TaskAttrConc_t;

        typedef struct _TaskAttrMon {
        } TaskAttrMon_t;

        typedef struct _TaskAttrOnce {
            xuint32_t timeout;
        } TaskAttrOnce_t;

        typedef struct _TaskResource {
            xint32_t taskResourceID;                //任务资源ID
            CallBack handleFunc;                    //处理函数
            CallBack errFunc;                       //异常函数
            void * userParam;                       //用户参数指针
            TaskResourceType_e taskResourceType;    //任务资源类型
            TaskResourceST_e taskResourceST;        //任务资源状态
            
            //与TaskResourceType_e绑定
            union {                                 
                TaskAttrTimeConc_t timeConc;
                TaskAttrTimeSer_t timeSer;
                TaskAttrConc_t conc;
                TaskAttrSer_t ser;
                TaskAttrMon_t mon;
                TaskAttrOnce_t once;
            } attr;
        } TaskResource_t;

        typedef struct _TaskNode {
            TaskResource_t * taskResource;
            TaskST_e taskST;                                                    //任务状态 
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


        xint32_t CheckAndUploadTimeTaskResource(xuint32_t & ver, std::vector<TaskResource_t *> & taskResource)
        {
            std::vector<TaskResource_t *> tmp;
            std::map<xint32_t, TaskResource_t *> timeResourceMap;
            std::map<xint32_t, TaskResource_t *>::iterator it;
            if (ver != m_timeTaskResourceVer)
            {
                ::pthread_mutex_lock(&m_lock);
                ver = m_timeTaskResourceVer;
                timeResourceMap = m_timeTaskResource;
                ::pthread_mutex_unlock(&m_lock);
                
                for (xint32_t iLoop = 0; iLoop < taskResource.size(); iLoop++)
                {
                    if (timeResourceMap.find(taskResource[iLoop]->taskResourceID) != timeResourceMap.end())
                    {
                        tmp.push_back(taskResource[iLoop]);
                        timeResourceMap.erase(taskResource[iLoop]->taskResourceID);
                    }
                }

                for (it = timeResourceMap.begin(); it !=  timeResourceMap.end(); it++)
                {
                    tmp.push_back(it->second);
                }

                return 1;
            }
            else
            {
                return 0;
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
            std::vector<TaskResource_t *> taskResource;
            xuint32_t ver;
            xint32_t interval = 10000;

            ver = m_timeTaskResourceVer - 100;

            while (threadInfo->threadST == THREAD_ST_RUN)
            {
                CheckAndUploadTimeTaskResource(ver, taskResource);
                for(xint32_t iLoop = 0; iLoop < taskResource.size(); iLoop++)
                {
                    switch(taskResource[iLoop]->taskResourceType)
                    {
                    case TASK_RESOURCE_TYPE_TIME_SER:
                        taskResource[iLoop]->attr.timeSer.leftTime -= interval;
                        if (taskResource[iLoop]->attr.timeSer.leftTime <= 0)
                        {
                            taskResource[iLoop]->attr.timeSer.leftTime = taskResource[iLoop]->attr.timeSer.interval;
                            AsyncHandleTask(taskResource[iLoop]->taskResourceID);
                        }
                        break;
                    case TASK_RESOURCE_TYPE_TIME_CONC:
                        taskResource[iLoop]->attr.timeConc.leftTime -= interval;
                        if (taskResource[iLoop]->attr.timeConc.leftTime <= 0)
                        {
                            taskResource[iLoop]->attr.timeConc.leftTime = taskResource[iLoop]->attr.timeConc.interval;
                            AsyncHandleTask(taskResource[iLoop]->taskResourceID);
                        }

                        break;
                    }
                }
                usleep(interval);
            }

            threadInfo->threadST = THREAD_ST_FINISHED;
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
            while(threadInfo->threadST == THREAD_ST_RUN)
            {
                sleep(1);
                //检测任务是否存在超时 等待状态的任务会直接移除并触发异常回调 
                //检测注销任务资源 确认无任务执行后释放资源

            }

            //管理线程一定是最后结束
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
            while(threadInfo->threadST == THREAD_ST_RUN)
            {
                sleep(1);
            }
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

            m_taskPoolST = TASK_POOL_ST_STOPPED;

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
        }

        xint32_t RegisterNormalTask(CallBack handleFunc, void * userParam, CallBack errFunc = NULL, bool IsSer = true, xuint32_t maxWaitTaskCount = 5, xint32_t timeout = 0)
        {
            TaskResource_t * taskResource = new TaskResource_t;

            xint32_t  resourceID = 0;
            ::pthread_mutex_lock(&m_lock);
            resourceID = GenTaskResourceID();
            m_taskResource[resourceID] = taskResource;

            taskResouorce->taskResourceID = resourceID;
            taskResouorce->handleFunc = handleFunc;
            taskResouorce->errFunc = errFunc;
            taskResouorce->userParam = userParam;
            taskResouorce->taskResourceST = TASK_RESOURCE_ST_FREE;

            if (IsSer)
            {
                taskResource->taskResourceType = TASK_RESOURCE_TYPE_SER;
                taskResource->attr.ser.timeout = timeout;
                taskResource->attr.ser.maxWaitTaskCount = maxWaitTaskCount;
                taskResource->attr.ser.waitCount = 0;
                taskResource->attr.ser.waitTaskNode = new FixLenList<struct _TaskNode *>;

                taskResource->attr.ser.waitTaskNode->Init(maxWaitTaskCount);

            }
            else
            {
                m_taskResource->taskResourceType = TASK_RESOURCE_TYPE_CONC;
                m_taskResource->attr.conc.timeout = timeout;
            }
            ::pthread_mutex_unlock(&m_lock);
            return handle;
        }

        xint32_t RegisterTimeTask(CallBack handleFunc, void * userParam, xuint32_t interval, CallBack errFunc = NULL, bool IsSer = true, xuint32_t maxWaitTaskCount = 5, xint32_t timeout = 0)
        {
            TaskResource_t * taskResource = new TaskResource_t;

            xint32_t resourceID = 0;
            ::pthread_mutex_lock(&m_lock);

            resourceID = GenTaskResourceID();
            m_taskResource[resourceID] = taskResource;

            taskResource->taskResourceID = resourceID;
            taskResource->handleFunc = handleFunc;
            taskResource->errFunc = errFunc;
            taskResource->userParam = userParam;
            taskResource->taskResourceST = TASK_RESOURCE_ST_FREE;

            if (IsSer)
            {
                taskResource->taskResourceType = TASK_RESOURCE_TYPE_TIME_SER;
                taskResource->attr.timeSer.timeout = timeout;

                taskResource->attr.timeSer.interval = interval;
                taskResource->attr.timeSer.leftTime = 0;
                taskResource->attr.timeSer.maxWaitTaskCount = maxWaitTaskCount;
                taskResource->attr.timeSer.waitCount = 0;
                taskResource->attr.timeSer.waitTaskNode = new FixLenList<struct _TaskNode *>;

                taskResource->attr.timeSer.waitTaskNode->Init(maxWaitTaskCount);

            }
            else
            {
                taskResource->taskResourceType = TASK_RESOURCE_TYPE_TIME_CONC;
                taskResource->attr.timeConc.timeout = timeout;
                taskResource->attr.timeSer.interval = interval;
                taskResource->attr.timeSer.leftTime = 0;
            }
            m_timeTaskResource[resourceID] = taskResource;
            ::pthread_mutex_unlock(&m_lock);
            return handle;
        }

        xint32_t RegisterMonTask(CallBack handleFunc, void * userParam, CallBack errFunc = NULL)
        {
            TaskResource_t * taskResource = new TaskResource_t;
            xint32_t  resourceID = 0;
            ::pthread_mutex_lock(&m_lock);
            resourceID = GenTaskResourceID();
            m_taskResource[resourceID] = taskResource;

            taskResource->taskResourceID = resourceID;
            taskResource->handleFunc = handleFunc;
            taskResource->errFunc = errFunc;
            taskResource->userParam = userParam;
            taskResource->taskResourceST = TASK_RESOURCE_ST_FREE;
            taskResource->taskResourceType = TASK_RESOURCE_TYPE_MON;
            monTaskResource[resourceID] = taskResource;
            ::pthread_mutex_unlock(&m_lock);
            //m_taskResource[handle]->attr
            return 0;
        }

        xint32_t UnRegisterTask(void * taskHandle)
        {
            TaskResource_t * taskResource = (TaskResource_t *)taskHandle;
            if (taskResource == NULL)
            {
                return -1;
            }

            pthread_mutex_lock(&m_lock);
            if (m_taskResource.find(taskResource->taskResourceID) != m_taskResource.end())
            {
                taskResource->taskResourceST = TASK_RESOURCE_ST_DROP;
                m_taskResource.erase(taskResource->taskResourceID);
                m_waitFreeTaskResource[taskResource->taskResourceID] = taskResource;
            }
            pthread_mutex_unlock(&m_lock);
            return 0;
        }

        xint32_t AsyncHandleTask(xint32_t handle, void * param, xint32_t lenOfParam)
        {
            TaskResource_t * taskResource = (TaskResource_t *)handel;
            TaskNode_t * taskNode = NULL;

            xint32_t timeout = 0;

            if (taskResource == NULL)
            {
                return -1;
            }

            timeout = taskResource
            ::pthread_mutex_lock(m_taskLock);
            while(!m_freeQueue.Empty())
            {

            }
            ::pthread_mutex_unlock(m_taskLock);
            return 0;
        }

        xint32_t SyncHandleTask()
        {
            //等待任务完成
            return 0;
        }

    public:

    };
}
#endif
