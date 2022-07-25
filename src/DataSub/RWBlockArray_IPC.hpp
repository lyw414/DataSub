#ifndef __LYW_CODE_RW_BLOCK_ARRAY_HPP_
#define __LYW_CODE_RW_BLOCK_ARRAY_HPP_
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include <functional>
#include <map>

namespace LYW_CODE
{
    class RWBlockArray_IPC
    {
    public:
        //data node list
        typedef struct _DataNode
        {
            struct _DataNode * next;
            unsigned char data[0];
        } DataNode_t;
        

        //index array 
        typedef struct _BlockHandle 
        {
            unsigned int index;
            unsigned int id;
        } BlockHandle_t;

        typedef struct _BlockArray
        {
            BlockHandle_t handle;
            DataNode_t * dataNode;
        } BlockArray_t;

    private:
        typedef struct  _RWBlock
        {
            unsigned int dataSize;

            unsigned int blockSize;

            unsigned int blockCount;

            unsigned int arraySize;
        
            unsigned int begin;

            unsigned int end;

            DataNode_t * freeBlockList;

            BlockArray_t * blockArray;

            unsigned char body[0];

        } RWBlock_t;

        typedef struct  _ShmInfo
        {
            pid_t  create;

            unsigned int ready;

            pthread_mutex_t lock;     

            pid_t lockPid;

            pthread_mutexattr_t lockAttr;

            pthread_cond_t cond;

            pthread_condattr_t condAttr;

            RWBlock_t rwBlock;

        } ShmInfo_t;

    private:
        pthread_mutex_t m_lock;

        //static std::map<key_t, int> m_referenceMap;
        
        ShmInfo_t * m_shmInfo;

        key_t m_shmKey;

        int m_shmID;

        int m_st;

        std::function <bool (void *)> m_isReadFinishFunc;

        //std::function <bool (void *)> m_timeoutHandleFunc;
    private:
        inline void Lock()
        {
            //struct timespec tv;
            //int res = 0;
            //::clock_gettime(CLOCK_REALTIME, &tv);
            //tv.tv_sec += 1;
            //while(::pthread_mutex_timedlock(&m_shmInfo->lock, &tv) == ETIMEDOUT)
            //{
            //    ::clock_gettime(CLOCK_REALTIME, &tv);
            //    tv.tv_sec += 1;
            //    if ((res = ::kill(m_shmInfo->lockPid, 0)) < 0)
            //    {
            //      UnLock();
            //    }
            //}
            //m_shmInfo->lockPid = getpid();
            ::pthread_mutex_lock(&m_shmInfo->lock);
        }

        inline void UnLock()
        {
            ::pthread_mutex_unlock(&m_shmInfo->lock);
        }

        inline unsigned int NextIndex(unsigned int current)
        {
            if (current + 1 == m_shmInfo->rwBlock.arraySize)
            {
                return 0;
            }
            else
            {
                return (current + 1);
            }
        }

        inline unsigned int BeforeIndex(unsigned int current)
        {
            if (current == 0)
            {
                return m_shmInfo->rwBlock.arraySize - 1;
            }
            else
            {
                return (current - 1);
            }
        }

        void RecoveryRB()
        {
            unsigned int next = m_shmInfo->rwBlock.begin;

            unsigned int before = m_shmInfo->rwBlock.end;

            DataNode_t * pNode = NULL;

            int st = 0;

            while(NextIndex(next) != before) 
            {
                if (st == 0)
                {
                    before = BeforeIndex(before);
                    if (m_isReadFinishFunc(m_shmInfo->rwBlock.blockArray[before].dataNode->data))
                    {

                        st = 1;
                    }
                    continue;
                }

                //find occupy
                if (st == 1)
                {
                    next = NextIndex(next);
                    if (!m_isReadFinishFunc(m_shmInfo->rwBlock.blockArray[next].dataNode->data))
                    {
                        st = 2;
                    }
                    else
                    {
                        //free 
                        pNode = m_shmInfo->rwBlock.blockArray[next].dataNode;
                        m_shmInfo->rwBlock.blockArray[next].dataNode = NULL;
                        pNode->next = m_shmInfo->rwBlock.freeBlockList;
                        m_shmInfo->rwBlock.freeBlockList = pNode;
                        m_shmInfo->rwBlock.begin = next;
                    }
                    continue;

                }

                if (st == 2)
                {
                    //free
                    pNode = m_shmInfo->rwBlock.blockArray[before].dataNode;
                    pNode->next = m_shmInfo->rwBlock.freeBlockList;
                    m_shmInfo->rwBlock.freeBlockList = pNode;

                    m_shmInfo->rwBlock.blockArray[before].dataNode = m_shmInfo->rwBlock.blockArray[next].dataNode;
                    m_shmInfo->rwBlock.blockArray[next].dataNode = NULL;
                    m_shmInfo->rwBlock.begin = next;

                    st = 0;

                    continue;
                }

            }
        }


    public:
        RWBlockArray_IPC(key_t shmKey)
        {
            m_shmInfo = NULL;

            m_shmKey = shmKey;

            m_shmID = -1;

            m_st = 0;

            m_isReadFinishFunc = NULL;

            ::pthread_mutex_init(&m_lock, NULL);
        }


        ~RWBlockArray_IPC()
        {
            DisConnect();
        }
        

        int Create(unsigned int blockSize, unsigned int blockCount)
        {
            int len = 0;

            int retry = 10;

            DataNode_t * pNode = NULL;

            ::pthread_mutex_lock(&m_lock);
            if (m_st != 0)
            {
                ::pthread_mutex_unlock(&m_lock);
                return -2;
            }

            //计算需要内存大小 
            len = sizeof(ShmInfo_t) + sizeof(BlockArray_t) * (blockCount + 2)  + (sizeof(DataNode_t) + blockSize) * blockCount;

            //连接至共享缓存
            m_shmID = ::shmget(m_shmKey, len, 0666 | IPC_CREAT | IPC_EXCL);

            if (m_shmID > 0)
            {
                //创建成功 初始化内存
                if ((m_shmInfo = (ShmInfo_t *)::shmat(m_shmID, NULL, 0)) == NULL)
                {
                    ::pthread_mutex_unlock(&m_lock);
                    return -1;
                }

                //格式化
                ::memset(m_shmInfo, 0x00, len);

                //设置未就绪
                m_shmInfo->ready = 0x0000;

                //初始化共享缓存
                m_shmInfo->create = ::getpid();

                //mutex
                pthread_mutexattr_init(&m_shmInfo->lockAttr);
                //进程共享
                pthread_mutexattr_setpshared(&m_shmInfo->lockAttr, PTHREAD_PROCESS_SHARED);

                pthread_mutex_init(&m_shmInfo->lock, &m_shmInfo->lockAttr);
 
                //cond 
                pthread_condattr_init(&m_shmInfo->condAttr);

                pthread_condattr_setpshared(&m_shmInfo->condAttr, PTHREAD_PROCESS_SHARED);
                //进程共享
                pthread_condattr_setpshared(&m_shmInfo->condAttr, PTHREAD_PROCESS_SHARED);
                //时钟属性
                pthread_condattr_setclock(&m_shmInfo->condAttr, CLOCK_MONOTONIC);

                pthread_cond_init(&m_shmInfo->cond, &m_shmInfo->condAttr);
                
                //读写块初始化
                m_shmInfo->rwBlock.dataSize = blockSize;
                m_shmInfo->rwBlock.blockSize = sizeof(DataNode_t) + blockSize;
                m_shmInfo->rwBlock.blockCount = blockCount;

                m_shmInfo->rwBlock.arraySize = blockCount + 2;

                m_shmInfo->rwBlock.begin = 0;
                m_shmInfo->rwBlock.end = 1;
                m_shmInfo->rwBlock.blockArray = (BlockArray_t *)m_shmInfo->rwBlock.body;
                for (int iLoop = 0; iLoop < m_shmInfo->rwBlock.arraySize; iLoop++)
                {
                    m_shmInfo->rwBlock.blockArray[iLoop].handle.index = 0;
                    m_shmInfo->rwBlock.blockArray[iLoop].handle.id = iLoop;
                }

                for (int iLoop = 0; iLoop < blockCount; iLoop++)
                {
                    if (iLoop == 0)
                    {
                        pNode = (DataNode_t *)(m_shmInfo->rwBlock.body + (sizeof(BlockArray_t) * (blockCount + 2)));
                        m_shmInfo->rwBlock.freeBlockList = pNode;
                    }

                    if (iLoop + 1 == blockCount)
                    {
                        //结束块
                        pNode->next = NULL; 
                    }
                    else
                    {
                        pNode->next = (DataNode_t *)((unsigned char *)pNode + m_shmInfo->rwBlock.blockSize);
                        pNode = pNode->next;
                    }
                }

                //设置就绪
                m_shmInfo->ready = 0x2A3B;
                ::shmdt(m_shmInfo);
                m_shmInfo = NULL;
                m_shmID = -1;

            }
            else
            {
                //已存在 则报错
                ::pthread_mutex_unlock(&m_lock);
                return -3;
            }
            
            //创建完成 并连接
            return 1;
        }

        /**
         * @brief                       连接共享缓存
         *
         * @param[in]shmKey             共享缓存键值
         * @param[in]blockSize          数据块大小 仅创建时生效
         * @param[in]shmKey             数据块数目 仅创建时生效
         * @param[in]shmKey             共享缓存键值
         *
         */
        int Connect(std::function <bool (void *)> isReadFinishFunc)
        {
            int retry = 10;
            ::pthread_mutex_lock(&m_lock);
            if (m_st == 0)
            {
                m_st = 2;
                ::pthread_mutex_unlock(&m_lock);

                m_isReadFinishFunc = isReadFinishFunc;

                //读写方式连接至共享缓存
                if ((m_shmID = ::shmget(m_shmKey, 0, 0)) < 0)
                {
                    m_st = 0;
                    return -3;
                }

                //连接至共享缓存
                if ((m_shmInfo = (ShmInfo_t *)::shmat(m_shmID, NULL, 0)) == NULL)
                {
                    m_st = 0;
                    return -1;
                }
                
                //等待初始化完成
                while(true)
                {
                    ::usleep(100000);
                    if (m_shmInfo->ready == 0x2A3B)
                    {
                        break;
                    }
                    retry--;
                    if (retry <= 0)
                    {
                        return -3;
                    }
                }
                m_st = 1;
                return 0;
            }
            else
            {
                ::pthread_mutex_unlock(&m_lock);
                return 0;
            }

        }

        int DisConnect()
        {
            ::pthread_mutex_lock(&m_lock);
            if (m_st == 1)
            {
                m_st = 0;
                ::pthread_mutex_unlock(&m_lock);
                ::shmdt(m_shmInfo);
                m_shmInfo = NULL;
                m_shmID = -1;
            }
            else
            {
                ::pthread_mutex_unlock(&m_lock);
            }

            return 0;
        }

        //释放共享缓存
        int Destroy(key_t key)
        {
            struct shmid_ds buf;
            int shmID = ::shmget(key, 0, IPC_EXCL);
            if (shmID > 0)
            {
                ::shmctl(shmID, IPC_RMID, &buf);
            }

            return 0;
        }

        //释放共享缓存 当前key
        int Destroy()
        {
            struct shmid_ds buf;
            int shmID = ::shmget(m_shmKey, 0, IPC_EXCL);
            if (shmID > 0)
            {
                ::shmctl(shmID, IPC_RMID, &buf);
            }
            return 0;
        }
 
        
        //数据块大小
        int BlockSize()
        {
            if (m_st == 1)
            {
                return m_shmInfo->rwBlock.dataSize;
            }
            return 0;
        }

        int Write(void * pData, int lenOfData, int timeout)
        {
            DataNode_t * pNode = NULL;
            unsigned int next = 0;
            int add = 1; 
            int leftTime = timeout;

            //未就绪
            if(m_st != 1)
            {
                return -1;
            }
            
            //数据超长
            if (m_shmInfo->rwBlock.dataSize < lenOfData)
            {
                return -2;
            }

            //获取空闲块
            while(true)
            {
                //::pthread_mutex_lock(&m_shmInfo->lock);
                Lock();
                pNode = m_shmInfo->rwBlock.freeBlockList;
                if (pNode != NULL)
                {
                    m_shmInfo->rwBlock.freeBlockList = pNode->next;
                }
                else
                {
                    RecoveryRB();

                    pNode = m_shmInfo->rwBlock.freeBlockList;
                    if (pNode != NULL)
                    {
                        m_shmInfo->rwBlock.freeBlockList = pNode->next;
                    }
                }

                //::pthread_mutex_unlock(&m_shmInfo->lock);
                UnLock();

                if (pNode == NULL)
                {
                    //队列满
                    ::pthread_cond_broadcast(&m_shmInfo->cond);
                    ::usleep(20 * add * 1000);
                    if (timeout != 0)
                    {
                        leftTime -= 20 * add;
                        if (leftTime <= 0)
                        {
                            return -4;
                        }
                    }

                    add = add * 2;
                }
                else
                {
                    break;
                }

            }
 
            //写数据
            ::memcpy(pNode->data, pData, lenOfData);

            //printf("w %p %s %d\n", pNode, (char *)pNode->data, m_shmInfo->rwBlock.end);

            //提交空闲块
            //::pthread_mutex_lock(&m_shmInfo->lock);
            Lock();
            //next = (m_shmInfo->rwBlock.end + 1) % m_shmInfo->rwBlock.arraySize;
            next = NextIndex(m_shmInfo->rwBlock.end);
            m_shmInfo->rwBlock.blockArray[m_shmInfo->rwBlock.end].dataNode = pNode;
            m_shmInfo->rwBlock.blockArray[m_shmInfo->rwBlock.end].handle.index++;
            m_shmInfo->rwBlock.end = next;
            //::pthread_mutex_unlock(&m_shmInfo->lock);
            UnLock();



            //notify
            ::pthread_cond_broadcast(&m_shmInfo->cond);
            return 0;
        }

        void * Read(BlockHandle_t * bBH, BlockHandle_t * rBH, std::function<bool(void *, unsigned int, void *)> isNeed, void * userParam)
        {
            unsigned int next = 0;

            unsigned int id = 0;

            void * data = NULL;

            if (bBH != NULL)
            {
                next = bBH->index;

                id = bBH->id;
            }
            else
            {
                id = 0;
                next = m_shmInfo->rwBlock.arraySize;
            }

            while(true)
            {
                //::pthread_mutex_lock(&m_shmInfo->lock);
                Lock();

                if (next >= m_shmInfo->rwBlock.arraySize)
                {
                    next = m_shmInfo->rwBlock.begin;
                }
                else if (m_shmInfo->rwBlock.begin < m_shmInfo->rwBlock.end)
                {
                    if ((next <= m_shmInfo->rwBlock.begin || next >= m_shmInfo->rwBlock.begin) || id != m_shmInfo->rwBlock.blockArray[next].handle.id)
                    {
                        next = m_shmInfo->rwBlock.begin;
                    }
                }
                else
                {
                    if ((next <= m_shmInfo->rwBlock.begin && next >= m_shmInfo->rwBlock.begin) || id != m_shmInfo->rwBlock.blockArray[next].handle.id)
                    {
                        next = m_shmInfo->rwBlock.begin;
                    }
                }


                next = NextIndex(next);

                while(next != m_shmInfo->rwBlock.end)
                {

                    if (isNeed(m_shmInfo->rwBlock.blockArray[next].dataNode->data, m_shmInfo->rwBlock.dataSize, userParam))
                    {
                        data = m_shmInfo->rwBlock.blockArray[next].dataNode->data;
                        if (rBH != NULL)
                        {
                            ::memcpy(rBH, &m_shmInfo->rwBlock.blockArray[next].handle, sizeof(BlockHandle_t));
                        }
                        break;
                    }

                    //next = (next + 1) % m_shmInfo->rwBlock.arraySize;
                    next = NextIndex(next);
                }

                //::pthread_mutex_unlock(&m_shmInfo->lock);
                UnLock();
                    //printf("R UNLOCK\n");

                if (data == NULL)
                {
                    ::pthread_mutex_lock(&m_shmInfo->lock);
                    //Lock()
                    //printf("R LOCK --- 1\n");
                    ::pthread_cond_wait(&m_shmInfo->cond, &m_shmInfo->lock);
                    //UnLock()
                    ::pthread_mutex_unlock(&m_shmInfo->lock);
                    next = m_shmInfo->rwBlock.arraySize;
                    //printf("R UNLOCK --- 2\n");
                }
                else
                {
                    break;

                }


            }
            return data;
        }
        /**
         * @brief   死锁解决方法
         *
         */
        void DoUnLock()
        {
            ::pthread_mutex_unlock(&m_shmInfo->lock);
        }


    };
}
#endif
