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

        typedef enum _CacheTableWriteLock {
            CACHE_TABLE_WRITE_LOCK,
            CACHE_TABLE_WRITE_UNLOCK,
        } CacheTableWriteLock_e;
        
        typedef struct _Node {
            NodeST_e st;
            xuint32_t rdRecord;
            xint32_t size;      //不包含Node_t
            xint32_t preIndex;
            xint32_t nextIndex;
            xint32_t lenOfData;
            xbyte_t dataPtr[0];
        } Node_t;
        
        typedef struct _CacheTable {

            pthread_mutex_t lock;           //进程锁
            pthread_mutexattr_t lockAttr;

            xint32_t size;          //CacheTable_t::ptr 的size
            xint32_t maxCacheSize;  //最大存放数据Size
            xuint32_t varifyID;     //校验ID
            xint32_t wIndex;        //写索引
            CacheTableWriteLock_e writeLock;    //写锁标识
                                
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
         * @param timeout           超时时间(ms) NULL:不做超时检测 非NULL:timeout会递减休眠时间
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

        
        xint32_t Read_Order(xint32_t ID, ProQuickTransReadRecord_t * readRecord,  void * readData, xint32_t sizeOfData, xint32_t * outLen, xint32_t timeout, xint32_t spinInterval)
        {
            CacheTable_t * pCacheTable = NULL;
            ProQuickTransReadRecord_t tmpReadRecord = {0};
            ProQuickTransReadRecord_t nextReadRecord = {0};

            xint32_t * leftTime = NULL;
            xint32_t wIndex = 0;
            xint32_t tmpIndex = 0;

            Node_t * pRNode = NULL;
            Node_t * pWNode = NULL;

            //0 未过期 1 过期
            xint32_t expiredTag = 0;

            xint32_t breakTag = 0;

            //合法性检测
            if (NULL == outLen)
            {
                RM_CBB_LOG_ERROR("PROQUICKTRANS","Read Error! outLen Param is NULL\n");
                return -2;
            }
        

            if (timeout != 0)
            {
                leftTime = &timeout;
            }
            
            if (IsInit() < 0)
            {
                RM_CBB_LOG_ERROR("PROQUICKTRANS","Read Error! Not Init\n");
                return -1;
            }


            if (ID < 0 || ID >= m_cachePool->cacheTableIndexArraySize)
            {
                RM_CBB_LOG_ERROR("PROQUICKTRANS","key[%02X] ID [%d] Error! maxID [%d]\n", m_key, ID, m_cachePool->cacheTableIndexArraySize);
            }

            pCacheTable = (CacheTable_t *)((xbyte_t *)m_cachePool  + m_cachePool->cacheTableIndex[ID]);


            //获取读节点 
            if (readRecord == NULL) 
            {
                tmpReadRecord.index = -1;
                tmpReadRecord.varifyID = 0;
            }
            else
            {
                ::memcpy(&tmpReadRecord, readRecord, sizeof(ProQuickTransReadRecord_t));
            }

            while(true)
            {
                expiredTag = 0;
                Lock(&pCacheTable->lock);
                wIndex = pCacheTable->wIndex;
                pWNode = (Node_t *)(pCacheTable->ptr + wIndex);
                if (tmpReadRecord.index < 0 || tmpReadRecord.index > pCacheTable->size - sizeof(Node_t))
                {
                    //无效索引 -- 读取旧值即可
                    expiredTag = 1;
                }
                else
                {
                    if (tmpReadRecord.index < wIndex)
                    {
                        //新一轮
                        if (pCacheTable->varifyID != tmpReadRecord.varifyID)
                        {
                            //节点过期
                            expiredTag = 1;
                        }
                    }
                    else if (tmpReadRecord.index == wIndex)
                    {
                        if (tmpReadRecord.varifyID + 1 == pCacheTable->varifyID)
                        {
                            //读取旧数据
                            if (pWNode->st == NODE_ST_WW)
                            {
                                //不可读 则一定为过期数据
                                expiredTag = 1;
                            }
                        }
                        else if (tmpReadRecord.varifyID == pCacheTable->varifyID)
                        {
                            //读取新数据
                        }
                        else
                        {
                            //节点过期
                            expiredTag = 1;
                        }
                    }
                    else if (tmpReadRecord.index > wIndex && tmpReadRecord.index < wIndex + pWNode->size + sizeof(Node_t)) 
                    {
                        //节点过期 -- 原有节点已经发生合并
                        expiredTag = 1;
                    }
                    else 
                    {
                        //旧一轮
                        if (pCacheTable->varifyID != tmpReadRecord.varifyID + 1)
                        {
                            //节点失效
                            expiredTag = 1;
                        }
                    }


                    if (1 == expiredTag) 
                    {
                        //过期节点 自动调整至最旧的节点
                        tmpReadRecord.varifyID = pCacheTable->varifyID;
                        tmpReadRecord.varifyID--;
                        tmpReadRecord.index = wIndex;
                    }


                    pRNode = (Node_t *)(pCacheTable->ptr + tmpReadRecord.index);
                    
                    //找到可读节点
                    tmpIndex = tmpReadRecord.index;
                    while(true)
                    {
                        if (pRNode->st == NODE_ST_NONE)
                        {
                            if (tmpIndex == wIndex && tmpReadRecord.varifyID >= pCacheTable->varifyID)
                            {
                                //完成一轮 或 varifyID异常
                                break;
                            }


                            tmpIndex = pRNode->nextIndex;
                            tmpReadRecord.index = tmpIndex;
                            pRNode = (Node_t *)(pCacheTable->ptr + pRNode->nextIndex);

                            if (0 == tmpIndex && tmpReadRecord.varifyID < pCacheTable->varifyID)
                            {
                                tmpReadRecord.varifyID++;
                            }

                        }
                        else
                        {

                            break;
                        }
                    }


                    if (pRNode->st == NODE_ST_RR)
                    {
                        //if (tmpReadRecord.index != pCacheTable->wIndex || tmpReadRecord.varifyID != pCacheTable->varifyID)
                        if ((tmpReadRecord.index < pCacheTable->wIndex && tmpReadRecord.varifyID == pCacheTable->varifyID) || (tmpReadRecord.index >= pCacheTable->wIndex && tmpReadRecord.varifyID + 1 == pCacheTable->varifyID))
                        {
                            __sync_fetch_and_add(&(pRNode->rdRecord), 1);
                            nextReadRecord.index = pRNode->nextIndex;
                            nextReadRecord.varifyID = tmpReadRecord.varifyID;
                            if (0 == nextReadRecord.index)
                            {
                                nextReadRecord.varifyID++;
                            }

                            UnLock(&pCacheTable->lock);
                            break;
                        }

                    }
                    
                }
                UnLock(&pCacheTable->lock);


                //自旋等待节点可读或失效
                while(true)
                {
                    breakTag = 0;
                    if (tmpReadRecord.index == pCacheTable->wIndex && tmpReadRecord.varifyID == pCacheTable->varifyID)
                    {
                        //当前读的节点是最新的数据 等待即可
                        if (DoSleep(spinInterval, leftTime) < 0)
                        {
                            RM_CBB_LOG_ERROR("PROQUICKTRANS","key[%02X] ID [%d]:: Read MSG Timeout[%d]!(wait write finished)\n", m_key, ID, timeout);
                            *outLen = -1;
                            return -2;
                        }

                        continue;
                    }

                    switch (pRNode->st)
                    {
                        case NODE_ST_WW:
                           //正在写的节点 需要检查节点是否过期
                            if (tmpReadRecord.index < pCacheTable->wIndex && tmpReadRecord.varifyID != pCacheTable->varifyID)
                            {
                                //过期
                                breakTag = 1;
                            }
                            else 
                            {
                                //脏读 异常再重试即可
                                wIndex = pCacheTable->wIndex;
                                if (wIndex < 0 || wIndex >= pCacheTable->maxCacheSize)
                                {
                                    break;
                                }
                                pWNode = (Node_t *)(pCacheTable->ptr + wIndex);

                                if (tmpReadRecord.index >= pCacheTable->wIndex && tmpReadRecord.index >= pCacheTable->wIndex + pWNode->size + sizeof(Node_t))
                                {
                                    //过期
                                    breakTag = 1;
                                }
                                else if (tmpReadRecord.varifyID + 1 != pCacheTable->varifyID)
                                {
                                    //过期
                                    breakTag = 1;
                                }
                            }
                            break;
                        case NODE_ST_RR:
                        case NODE_ST_NONE:
                        default:
                            breakTag = 1;
                            break;
                    }

                    if (breakTag == 1)
                    {
                        break;
                    }

                    if (DoSleep(spinInterval, leftTime) < 0)
                    {
                        RM_CBB_LOG_ERROR("PROQUICKTRANS","key[%02X] ID [%d]:: Read MSG Timeout[%d]!(wait write finished)\n", m_key, ID, timeout);
                        *outLen = -1;
                        return -2;
                    }

                }
            }

            //读数据
            *outLen = pRNode->lenOfData;
            if (pRNode->lenOfData > sizeOfData || NULL == readData) 
            {
                RM_CBB_LOG_ERROR("PROQUICKTRANS","key[%02X] ID [%d]:: Data Buffer Not Enough bufferSize [%d] needSize [%d]!\n", m_key, ID, sizeOfData, pRNode->size);
                return -3;
            }
            else
            {
                ::memcpy(readRecord, &nextReadRecord, sizeof(ProQuickTransReadRecord_t));

                ::memcpy((xbyte_t *)readData, pRNode->dataPtr, pRNode->lenOfData);
            }

            __sync_fetch_and_sub(&(pRNode->rdRecord), 1);


            return 0;

        }

        xint32_t Read_Newest(xint32_t ID, ProQuickTransReadRecord_t * readRecord,  void * readData, xint32_t sizeOfData, xint32_t * outLen, xint32_t timeout, xint32_t spinInterval)
        {
            return 0;
        }


        xint32_t Read_Oldest(xint32_t ID, ProQuickTransReadRecord_t * readRecord,  void * readData, xint32_t sizeOfData, xint32_t * outLen, xint32_t timeout, xint32_t spinInterval)
        {
            return 0;
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
                    table->writeLock = CACHE_TABLE_WRITE_UNLOCK;

                    node->st = NODE_ST_NONE;
                    node->rdRecord = 0;
                    node->size = cfgArray[iLoop].size;
                    node->preIndex = 0;
                    node->nextIndex = 0;
                }

                m_cachePool->st = 1;

                RM_CBB_LOG_WARNING("PROQUICKTRANS","ProQuickTrans Init key[%02X] (create) Success\n", m_key);
            }
            else
            {
                //共享缓存已存在 连接共享缓存
                if ((m_shmID = ::shmget(m_key, 0, 0)) < 0)
                { 
                    RM_CBB_LOG_ERROR("PROQUICKTRANS", "key [%02X] (connect) get Failed!\n", m_key);
                    return -2;
                }

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
            xint32_t needLen = 0;
            xint32_t wIndex = 0;
            xint32_t nextIndex = 0;

            xint32_t cacheSize = 0;
            

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

            if (lenOfData <= 0)
            {
                lenOfData = 0;
            }

            needLen = lenOfData + sizeof(Node_t);

            if (ID < 0 || ID >= m_cachePool->cacheTableIndexArraySize)
            {
                RM_CBB_LOG_ERROR("PROQUICKTRANS","key[%02X] ID [%d] Error! maxID [%d]\n", m_key, ID, m_cachePool->cacheTableIndexArraySize);
            }

            pCacheTable = (CacheTable_t *)((xbyte_t *)m_cachePool  + m_cachePool->cacheTableIndex[ID]);

            if (lenOfData > pCacheTable->maxCacheSize)
            {
                RM_CBB_LOG_ERROR("PROQUICKTRANS","key[%02X] ID [%d]:: MSG Too long ! MSGLen [%d] maxCacheSize [%d]\n", m_key, ID, lenOfData, pCacheTable->maxCacheSize);
            }


            //获取写节点
            while (true)
            {
                //独占写节点分配操作    
                if (pCacheTable->writeLock == CACHE_TABLE_WRITE_UNLOCK)
                {
                    Lock(&pCacheTable->lock);
                    if (pCacheTable->writeLock == CACHE_TABLE_WRITE_UNLOCK)
                    {
                        pCacheTable->writeLock = CACHE_TABLE_WRITE_LOCK;
                        UnLock(&pCacheTable->lock);
                        
                        //等待写操作完成
                        wIndex = pCacheTable->wIndex;
                        pNode = tmpNode = (Node_t *)(pCacheTable->ptr + wIndex);
                        nextIndex = pNode->nextIndex;
                        cacheSize = 0;

                        while(true)
                        {
                            if (tmpNode->st == NODE_ST_WW)
                            {
                                if (DoSleep(spinInterval, leftTime) < 0)
                                {
                                    //放开申请写节点操作
                                    pCacheTable->writeLock = CACHE_TABLE_WRITE_UNLOCK;
                                    RM_CBB_LOG_ERROR("PROQUICKTRANS","key[%02X] ID [%d]:: Write MSG Timeout[%d]!(wait write finished)\n", m_key, ID, timeout);
                                    return -1;
                                }
                                continue;
                            }

                            cacheSize += tmpNode->size + sizeof(Node_t);
                            if (cacheSize >= needLen)
                            {
                                //取到足够长的连续空间 更新写节点
                                Lock(&pCacheTable->lock);
                                pNode->st = NODE_ST_WW;
                                pNode->nextIndex = tmpNode->nextIndex;
                                pNode->size = cacheSize - sizeof(Node_t);

                                pCacheTable->wIndex = tmpNode->nextIndex;
                                tmpNode = (Node_t *)(pCacheTable->ptr + tmpNode->nextIndex);
                                tmpNode->preIndex = wIndex;
                                if (pCacheTable->wIndex == 0)
                                {
                                    pCacheTable->varifyID++;
                                }
                                UnLock(&pCacheTable->lock);
                                break;
                            }

                            if (tmpNode->nextIndex == 0)
                            {
                                //剩余连续长度不够长 则丢弃该段数据
                                Lock(&pCacheTable->lock);
                                pNode->st = NODE_ST_NONE;
                                pNode->nextIndex = tmpNode->nextIndex;
                                pNode->size = cacheSize - sizeof(Node_t);

                                pCacheTable->wIndex = tmpNode->nextIndex;
                                pCacheTable->varifyID++;
                                tmpNode = (Node_t *)(pCacheTable->ptr + tmpNode->nextIndex);
                                tmpNode->preIndex = wIndex;
                                UnLock(&pCacheTable->lock);

                                //从新的连续节点开始继续获取
                                pNode = tmpNode;
                                wIndex = 0;

                                nextIndex = pNode->nextIndex;
                                cacheSize = 0;
                                continue;
                            }
                            
                            tmpNode = (Node_t *)(pCacheTable->ptr + tmpNode->nextIndex);
                        }

                        break;
                    }
                    UnLock(&pCacheTable->lock);
                }

                if (DoSleep(spinInterval, leftTime) < 0)
                {

                    RM_CBB_LOG_ERROR("PROQUICKTRANS","key[%02X] ID [%d]:: Write MSG Timeout[%d]!(wait write operator)\n", m_key, ID, timeout);
                    return -1;
                }

            }
            

            //等待节点内读操作清空 并进行写
            pCacheTable->writeLock = CACHE_TABLE_WRITE_UNLOCK;

            //if (wIndex == 0)
            //{
            //    Lock(&pCacheTable->lock);
            //    pCacheTable->varifyID++;
            //    UnLock(&pCacheTable->lock);
            //}

            
            //首个节点(首个节点的nextIndex已经修改了，需要单独处理）
            while(true)
            {
                //pNode 等待
                if (pNode->rdRecord == 0)
                {
                    //写已经完成

                    break;
                }

                if (DoSleep(spinInterval, leftTime) < 0)
                {
                    Lock(&pCacheTable->lock);
                    pNode->st = NODE_ST_NONE;
                    UnLock(&pCacheTable->lock);
                    RM_CBB_LOG_ERROR("PROQUICKTRANS","key[%02X] ID [%d]:: wait first Node read finished timeout [%d]!\n", m_key, ID, timeout);
                    return -1;
                }
            }
            
            //后续节点
            tmpNode = (Node_t *)(pCacheTable->ptr + nextIndex);
            while (nextIndex != pNode->nextIndex)
            {
                nextIndex = tmpNode->nextIndex;

                //pNode 等待
                if (tmpNode->rdRecord == 0)
                {
                    //写已经完成
                    tmpNode = (Node_t *)(pCacheTable->ptr + nextIndex);
                    continue;
                }

                if (DoSleep(spinInterval, leftTime) < 0)
                {
                    Lock(&pCacheTable->lock);
                    pNode->st = NODE_ST_NONE;
                    UnLock(&pCacheTable->lock);
                    RM_CBB_LOG_ERROR("PROQUICKTRANS","key[%02X] ID [%d]:: wait left Node read finished timeout [%d]!\n", m_key, ID, timeout);
                    return -1;
                }
            }

            //对长节点进行拆分 -- 此处不用加锁
            if (cacheSize - needLen > sizeof(Node_t))
            {
                tmpNode = (Node_t * )((xbyte_t *)pNode + needLen);
                tmpNode->nextIndex = nextIndex;
                tmpNode->preIndex = wIndex;
                tmpNode->st = NODE_ST_NONE;
                tmpNode->size = cacheSize - needLen - sizeof(Node_t);
                tmpNode->rdRecord = 0;

                tmpNode = (Node_t * )(pCacheTable->ptr + tmpNode->nextIndex);
                if (pNode->nextIndex == pNode->preIndex)
                {
                    pNode->nextIndex = wIndex + needLen;
                    pNode->preIndex = wIndex + needLen;
                }
                else
                {
                    pNode->nextIndex = wIndex + needLen;
                    Lock(&pCacheTable->lock);
                    tmpNode->preIndex = wIndex + needLen;
                    UnLock(&pCacheTable->lock);
                }

                pNode->size = lenOfData;
            }
            else
            {
                //pNode->size = cacheSize - sizeof(Node_t);
            }

            pNode->lenOfData = lenOfData;


            //写数据
            ::memcpy(pNode->dataPtr, data, lenOfData);

            pNode->st = NODE_ST_RR;

            return 0;
        }

        
        xint32_t Read(xint32_t ID, ProQuickTransReadRecord_t * readRecord,  void * readData, xint32_t sizeOfData, xint32_t * outLen, ProQuickTranReadMode_e readMode = PRO_READ_ORDER, xint32_t timeout = 0, xint32_t spinInterval = 1)
        {
            switch(readMode)
            {
                case PRO_READ_ORDER:
                    return Read_Order(ID, readRecord, readData, sizeOfData, outLen, timeout, spinInterval);
                case PRO_READ_NEWEST_ONLY:
                    return Read_Newest(ID, readRecord, readData, sizeOfData, outLen, timeout, spinInterval);
                case PRO_READ_OLDEST_ONLY:
                    return Read_Oldest(ID, readRecord, readData, sizeOfData, outLen, timeout, spinInterval);
                default:
                    RM_CBB_LOG_ERROR("PROQUICKTRANS","key[%02X] ID [%d]:: unkonw read mode\n", m_key, ID);
                    return -1;
            }

            return -1;
        }


    };
}
#endif
