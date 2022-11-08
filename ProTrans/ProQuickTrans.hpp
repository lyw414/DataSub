#ifndef __RM_CBB_API_PRO_QUICK_TRANS_FILE_HPP__
#define __RM_CBB_API_PRO_QUICK_TRANS_FILE_HPP__
#include "streamaxcomdev.h" 
#include "ProQuickTransDefine.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>

#include <map>

#include "streamaxlog.h"

namespace RM_CODE 
{
    class ProQuickTrans {
    private:
        typedef enum _NodeST {
            NODE_ST_WW,     //正在写，禁止读与写，读写均阻塞至该状态
            NODE_ST_RR,     //读就绪，可读可写
            NODE_ST_NONE,   //无效，可写不可读，读操作直接跳过该节点
        } NodeST_e;
        
        typedef struct _Node {
            NodeST_e st;
            xuint32_t rdRecord;
            xint32_t size;      //不包含Node_t
            xint32_t preIndex;
            xint32_t nextIndex;
            xbyte_t dataPtr[0];
        } Node_t;
        
        typedef struct _CacheTable {
            pthread_mutex_t lock;           //进程锁
            pthread_mutexattr_t lockAttr;

            xint32_t size;          //CacheTable_t::ptr 的size
            xint32_t maxCacheSize;  //最大存放数据Size
            xuint32_t varifyID;     //校验ID
            xint32_t wIndex;        //写索引
                                
            xbyte_t ptr[0];     //Node_t 链表起始地址
        } CacheTable_t;

        typedef struct _CachePool {
            xint32_t totalSize;
            xint32_t st;
            xint32_t cacheTableIndexArraySize;
            xint32_t cacheTableIndex[0];
        } CachePool_t;
    
    private:
        pthread_mutex_t m_lock;

        key_t m_key;
        
        xint32_t m_st;

        xint32_t m_shmID;

        CachePool_t * m_cachePool;

    private:
        void Lock(pthread_mutex_t * lock)
        {
            ::pthread_mutex_lock(lock);
        }

        void UnLock(pthread_mutex_t * lock)
        {
            ::pthread_mutex_unlock(lock);
        }
        
        /**
         * @brief               检测是否完成初始
         *
         * @return  >= 0        成功
         *          <  0        失败
         */
        xint32_t IsInit()
        {
            if (m_st != 1 || NULL == m_cachePool)
            {
                return -1;
            }
            
            return 0;
        }

        /**
         * @brief                   休眠并超时检测
         *
         * @param times             休眠时间(ms) 0:使用yield切出线程
         * @param timeout           超时时间(ms) NULL:不做超时检测
         *
         * @return >= 0             未超时
         *                          超时
         */
        inline xint32_t DoSleep(xint32_t times, xint32_t * timeout)
        {
            if (times > 0)
            {
                usleep(times * 1000);
                if(NULL != timeout)
                {
                    *timeout -= times;
                    return (*timeout);
                }
                else
                {
                    return 0;
                }
            }
            else
            {
                ::sched_yield();
                if(NULL != timeout)
                {
                    *timeout -= 1;
                    return (*timeout);
                }
                else
                {
                    return 0;
                }
            }
        }

        

    public:
        ProQuickTrans()
        {
            m_key = -1;
            ::pthread_mutex_init(&m_lock, NULL);
            m_st = 0;
            m_shmID = -1;
            m_cachePool = NULL;
        }

        ~ProQuickTrans()
        {
        }

        /**
         * @brief   初始化
         *
         * @param key               共享缓存键值
         * @param cfgArray          创建配置 - 若共享缓存键值已创建则该配置不会生效
         * @param cfgNum            配置记录条数
         *
         * @return  >= 0            成功 0 创建成功 1 连接成功
         *          <  0            失败 错误码
         */
        xint32_t Init(key_t key, ProQuickTransCFG_t * cfgArray, xint32_t cfgNum)
        {
            xint32_t totalSize;

            xint32_t retryTimes = 5;
            
            xint32_t maxID = -1;

            xint32_t index = 0;

            //状态检测
            ::pthread_mutex_lock(&m_lock);
            if (1 == m_st)
            {
                ::pthread_mutex_unlock(&m_lock);
                return -1;
            }
            ::pthread_mutex_unlock(&m_lock);

            m_st = 1;

            m_key = key;

            RM_CBB_LOG_WARNING("PROQUICKTRANS","ProQuickTrans Init key[%02X] begin ......\n", m_key);

            
            //统计需要申请的空间大小
            totalSize = sizeof(CachePool_t);
            if (NULL != cfgArray && cfgNum > 0)
            {
                for (xint32_t iLoop = 0; iLoop < cfgNum; iLoop++)
                {
                    if (maxID < cfgArray[iLoop].id)
                    {
                        maxID = cfgArray[iLoop].id;
                    }
                    totalSize += sizeof(CacheTable_t);
                    totalSize += sizeof(Node_t); //初始至少包含一个节点
                    totalSize += cfgArray[iLoop].size;
                }
            }

            totalSize += sizeof(xint32_t) * (maxID + 1);
            
            //创建共享缓存
            m_shmID = ::shmget(m_key, totalSize, 0666 | IPC_CREAT | IPC_EXCL);

            if (m_shmID >= 0)
            {
                //共享缓存不存在 创建成功 连接并初始化共享缓存
                if ((m_cachePool = (CachePool_t *)::shmat(m_shmID, NULL, 0)) == NULL)
                {
                    RM_CBB_LOG_ERROR("PROQUICKTRANS", "key [%02X] (create) Connect Failed!\n", m_key);
                    m_st = 0;
                    return -2;
                }

                //初始化
                ::memset(m_cachePool, 0x00, totalSize);

                m_cachePool->cacheTableIndexArraySize = maxID + 1;

                m_cachePool->totalSize = totalSize;

                index = sizeof(CachePool_t) + sizeof(xint32_t) * (maxID + 1);

                for (xint32_t iLoop = 0; iLoop < cfgNum; iLoop++)
                {
                    CacheTable_t * table = NULL;
                    Node_t * node = NULL;

                    m_cachePool->cacheTableIndex[cfgArray[iLoop].id] = index;
                    table = (CacheTable_t *)((xbyte_t *)m_cachePool + index);
                    node = (Node_t *)(table->ptr);

                    index += sizeof(CacheTable_t) + sizeof(Node_t) + cfgArray[iLoop].size;

                    //进程锁 
                    pthread_mutexattr_init(&table->lockAttr);
                    pthread_mutexattr_setpshared(&table->lockAttr, PTHREAD_PROCESS_SHARED);
                    pthread_mutex_init(&table->lock, &table->lockAttr);
                    table->varifyID = 1;
                    table->wIndex = 0;
                    table->size = cfgArray[iLoop].size + sizeof(Node_t);
                    table->maxCacheSize = cfgArray[iLoop].size;

                    node->st = NODE_ST_NONE;
                    node->rdRecord = 0;
                    node->size = cfgArray[iLoop].size;
                    node->preIndex = 0;
                    node->nextIndex = 0;
                }

                RM_CBB_LOG_WARNING("PROQUICKTRANS","ProQuickTrans Init key[%02X] (create) Success\n", m_key);
            }
            else
            {
                //共享缓存已存在 连接共享缓存
                if ((m_cachePool = (CachePool_t *)::shmat(m_shmID, NULL, 0)) == NULL)
                {
                    RM_CBB_LOG_ERROR("PROQUICKTRANS", "key [%02X] (connect) Connect Failed!\n", m_key);
                    m_st = 0;
                    return -2;
                }

                //等待资源就绪
                while (true) 
                {
                    if (1 == m_cachePool->st)
                    {
                        break;
                    }
                    retryTimes--;
                    
                    if (retryTimes <= 0)
                    {
                        RM_CBB_LOG_ERROR("PROQUICKTRANS", "key [%02X] (connect) wait shm ready timeout!\n", m_key);
                        m_st = 0;
                        return -3;
                    }
                    ::usleep(500000);
                }

                RM_CBB_LOG_WARNING("PROQUICKTRANS","ProQuickTrans Init key[%02X] (connect) Success\n", m_key);

            }

            return 0;
        }
        
        
        /**
         * @brief   反初始化 - 是否资源
         *
         * @return  >= 0            成功
         *          <  0            失败
         */
        xint32_t UnInit()
        {
            Lock(&m_lock);
            m_st = 0;
            if (m_cachePool != NULL)
            {
                ::shmdt(m_cachePool);
                m_cachePool = NULL;
            }
            UnLock(&m_lock);
            return 0;
        }

        /**
         * @brief   销毁共享缓存 - 键值
         *
         * @param key               共享缓存键值
         *
         * @return  >= 0            成功
         *          <  0            失败
         */
        xint32_t Destroy(key_t key)
        {
            struct shmid_ds buf;
            xint32_t shmID = ::shmget(key, 0, IPC_EXCL);
            if (shmID >= 0)
            {
                ::shmctl(shmID, IPC_RMID, &buf);
            }
            return 0;
        }

        
        /**
         * @brief 
         *
         * @param ID                类型ID
         * @param data              数据
         * @param lenOfData         数据长度
         * @param timeout           超时(ms) < 0: 立即返回 0: 阻塞 >0 : 超时时间
         * @param spinInterval      自旋间隔(ms) 
         *
         * @return  >= 0            成功 
         *          <  0            失败
         */
        xint32_t Write(xint32_t ID, void * data, xint32_t lenOfData, xint32_t timeout = 0, xuint32_t spinInterval = 1)
        {
            CacheTable_t * pCacheTable = NULL;
            Node_t * pNode = NULL; 
            Node_t * tmpNode = NULL;

            xint32_t * leftTime = NULL;
            xint32_t needNodeCount = 0;
            xint32_t leftCacheSize = 0;
            xint32_t maxCacheSize = 0;
            xint32_t wIndex = 0;
            xint32_t nextIndex = 0;
            
            //跳段检测 2：跳段未重试 1：跳段重试 0：跳段重试完成
            xint32_t skipCheck = 1;

            //0 允许跳段 1 禁止跳段
            xint32_t skipTag = 0;

            if (timeout != 0)
            {
                leftTime = &timeout;
            }
            
            //合法性检测
            if (IsInit() < 0)
            {
                RM_CBB_LOG_ERROR("PROQUICKTRANS","Wrte Error! Not Init\n");
                return -1;
            }
            
            if (ID < 0 || ID >= m_cachePool->cacheTableIndexArraySize)
            {
                RM_CBB_LOG_ERROR("PROQUICKTRANS","key[%02X] ID [%d] Error! maxID [%d]\n", m_key, ID, m_cachePool->cacheTableIndexArraySize);
            }

            pCacheTable = (CacheTable_t *)((xbyte_t *)m_cachePool  + m_cachePool->cacheTableIndex[ID]);

            if (lenOfData > pCacheTable->maxCacheSize)
            {
                RM_CBB_LOG_ERROR("PROQUICKTRANS","key[%02X] ID [%d]:: MSG Too long ! MSGLen [%d] maxCacheSize [%d]\n", m_key, ID, lenOfData, pCacheTable->maxCacheSize);
            }

            //获取写
            while (true)
            {
                //脏读 -- 提高命中率 减少锁的次数
                wIndex = pCacheTable->wIndex;
                if (wIndex < 0 || wIndex >= pCacheTable->maxCacheSize)
                {
                    //wIndex 为脏数据继续读
                    if (DoSleep(spinInterval, leftTime) < 0)
                    {
                        RM_CBB_LOG_ERROR("PROQUICKTRANS","key[%02X] ID [%d]:: Write MSG Timeout[%d]\n", m_key, ID, timeout);
                        return -1;
                    }
                    continue;
                }

                pNode = (Node_t *)(pCacheTable->ptr + wIndex);

                if (pNode->st != NODE_ST_RR && pNode->st != NODE_ST_NONE)
                {
                    //wIndex 为脏数据继续读
                    if (DoSleep(spinInterval, leftTime) < 0)
                    {
                        RM_CBB_LOG_ERROR("PROQUICKTRANS","key[%02X] ID [%d]:: Write MSG Timeout[%d]\n", m_key, ID, timeout);
                        return -1;
                    }
                    continue;
                }
                
                //锁数据 
                Lock(&pCacheTable->lock);
                wIndex = pCacheTable->wIndex;
                pNode = (Node_t *)(pCacheTable->ptr + wIndex);
                if (pNode->st == NODE_ST_WW)
                {
                    wIndex = -1;
                }
                else
                {
                    pNode->st = NODE_ST_WW;
                }
                UnLock(&pCacheTable->lock);

                maxCacheSize = pCacheTable->maxCacheSize - wIndex;
                //新的读操作会阻塞在 wIndex 节点 后续写操作仅修仅在写完成后将ST维护为 RR/NONE 检测ST状态为非 WW 后进行读操作是安全的
                
                if (maxCacheSize >= lenOfData + sizeof(Node_t))
                {
                    //等待所有读完成 或 超时
                }


                while(true)
                {
                    wIndex = pCacheTable->wIndex;
                    tmpNode = pNode = (Node_t *)(pCacheTable->ptr + wIndex);
                    
                    if (pNode->st == NODE_ST_WW)
                    {
                        wIndex = -1;
                        break;
                    }
                    
                    //最大剩余数据缓存
                    //检测剩余节点状态
                    leftCacheSize = 0;
                    needNodeCount = 0; 
                    skipTag = 0;

                    while (true)
                    {
                        needNodeCount++; 
                        if (tmpNode->st == NODE_ST_WW)
                        {
                            skipTag = 1;
                            break;
                        }

                        //循环读取直到数据截断或读取完成
                        leftCacheSize += sizeof(Node_t) + tmpNode->size;
                        if (leftCacheSize >= lenOfData + sizeof(Node_t))
                        {
                            //取到合适数据
                            break;
                        }
                        
                        if (tmpNode->nextIndex != 0)
                        {
                            tmpNode = (Node_t *)(pCacheTable->ptr + tmpNode->nextIndex);
                        }
                        else
                        {
                            break;
                        }
                    }

                    
                    if (maxCacheSize >= lenOfData + sizeof(Node_t))
                    {
                        
                    }
                    else
                    {

                    }

                    pNode = (Node_t *)(pCacheTable->ptr + wIndex);
                    tmpNode = pNode;

                    skipTag = 0;
                    leftCacheSize = 0;
                    needNodeCount = 0; 

                    if (pNode->st == NODE_ST_WW)
                    {
                        //起始节点不可写跳出锁 自旋等待
                        wIndex = -1;
                        break;
                    }

                    
                    //解析至BUFF末尾(使用连续区域) 或 长度足够

                    if (leftCacheSize >= lenOfData + sizeof(Node_t))
                    {
                        //剩余连续空间足够长
                        if (0 == skipTag)
                        {
                            pCacheTable->wIndex = tmpNode->nextIndex;
                            if (wIndex == 0)
                            {
                                //跨段 刷新ID
                                pCacheTable->varifyID++;
                            }

                            tmpNode = (Node_t *)(pCacheTable->ptr + tmpNode->nextIndex);
                        }
                        break;
                    }
                    else
                    {
                        //剩余连续空间不够长
                        if (0 == skipTag && 2 == skipCheck)
                        {
                            pCacheTable->wIndex = 0;
                            skipCheck = 1;
                        }
                        else
                        {
                            //重试后长度依旧不够 -- 该逻辑不会出现 若出现该打印为设计缺陷
                            RM_CBB_LOG_ERROR("PROQUICKTRANS","key[%02X] ID [%d]:: maxLen Not Enough! maxSize [%d] needLen[%d]! Design Error!\n", m_key, ID, pCacheTable->maxCacheSize, lenOfData);
                            skipCheck = 0;
                            wIndex =-1;
                            break;
                        }
                    }
                }
                UnLock(&pCacheTable->lock);

                if (wIndex != -1)
                {
                    //找到可用数据
                    break;
                }
            }

            //等待读完成
            

            //写操作

            //分割写块

            return 0;
        }
    };
}
#endif
