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
            NODE_ST_CHECK   //待检测节点 需要检测是否内部节点存待写 读操作 均无之后则可以设置为 NODE_ST_NONE

        } NodeST_e;
        
        typedef struct _Node {
            NodeST_e st;                //节点状态
            xuint32_t rdRecord;         //读计数
            xint32_t size;              //不包含Node_t
            xint32_t len;               //数据长度 
            xint32_t preIndex;
            xint32_t nextIndex;
            xint32_t checkNextIndex;    //缓存块合并nextIndex

            xbyte_t dataPtr[0];
        } Node_t;
        
        typedef struct _CacheTable {
            pthread_mutex_t lock;           //进程锁
            pthread_mutexattr_t lockAttr;

            pthread_mutex_t wLock;           //写锁
            pthread_mutexattr_t wLockAttr;

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
         * @brief                       读取数据
         *
         * @param ID                    类型ID
         * @param readIndex             读索引
         * @param data                  读数据缓存
         * @param sizeOfData            读数据缓存大小 
         * @param outLen                数据长度
         * @param timeout               超时时间 ms
         * @param spinInterval          自旋间隔 ms
         *
         * @return  >= 0                成功
         *          <  0                失败 
         */
        xint32_t Read_Order(xint32_t ID, ProQuickTransReadIndex_t * readIndex, void * data, xint32_t sizeOfData, xint32_t * outLen,  xint32_t timeout, xint32_t spinInterval)
        {
            CacheTable_t * pCacheTable = NULL;
            
            Node_t * pNode = NULL;

            xint32_t rIndex = 0;

            ProQuickTransReadIndex_t RI;

            ProQuickTransReadIndex_t NRI;

            xint32_t * leftTime = NULL;
            
            //0 过期索引 1 有效索引
            xint32_t isInvalid = 0;

            if (timeout != 0)
            {
                leftTime = &timeout;
            }
 
            //合法性检测
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

            if (NULL == readIndex)
            {
                RI.index = -1;
            }
            else
            {
                ::memcpy(&RI, readIndex, sizeof(ProQuickTransReadIndex_t));
            }

            while(true)
            {
                isInvalid = 1;
                Lock(&pCacheTable->lock);
                //读索引检测
                if (RI.index < 0 || RI.index >= pCacheTable->maxCacheSize || RI.ID != ID)
                {
                    //索引范围错误
                    RI.ID = ID;
                    isInvalid = 0;
                }
                
                //检测校验ID是否合法
                if (RI.index < pCacheTable->wIndex) 
                {
                    if (RI.varifyID != pCacheTable->varifyID)
                    {
                        isInvalid = 0;
                    }
                }
                else 
                {
                    rIndex = pCacheTable->wIndex;
                    pNode = (Node_t *)(pCacheTable->ptr + rIndex);
                    if (RI.index == rIndex)
                    {
                        if (RI.varifyID  + 1 != pCacheTable->varifyID && RI.varifyID != pCacheTable->varifyID)
                        {
                            //既不是最旧值 也不是最新值
                            isInvalid = 0;
                        }
                    }
                    else if (RI.index < rIndex + pNode->size + sizeof(Node_t))
                    {
                        //处于合并块中 说明已经失效了
                        isInvalid = 0;
                    }
                    else
                    {
                        if (RI.varifyID  + 1 != pCacheTable->varifyID)
                        {
                            isInvalid = 0;
                        }
                    }
                }
                
                if (0 == isInvalid)
                {
                    rIndex = pCacheTable->wIndex;

                    RI.index = rIndex;
                    RI.varifyID = pCacheTable->varifyID - 1;
                }
                else
                {
                    rIndex = RI.index;
                }

                pNode = (Node_t *)(pCacheTable->ptr + rIndex);
                //从当前节点开始找到一个可以读的节点
                while(true)
                {
                    if (pNode->st != NODE_ST_NONE) 
                    {
                        break;
                    }

                    if (rIndex == pCacheTable->wIndex && RI.varifyID == pCacheTable->varifyID)
                    {
                        //跳转到最新的节点了
                        break;
                    }

                    rIndex = pNode->nextIndex;
                    pNode = (Node_t *)(pCacheTable->ptr + rIndex);
                    if (rIndex == 0)
                    {
                        RI.varifyID = pCacheTable->varifyID;
                    }
                } 

                RI.index = rIndex;

                if (pNode->st == NODE_ST_RR && (RI.index != pCacheTable->wIndex || RI.varifyID != pCacheTable->varifyID))
                {
                    //节点状态可读 设置读计数
                    NRI.index = pNode->nextIndex;
                    NRI.ID = RI.ID;
                    if (NRI.index == 0)
                    {
                        NRI.varifyID = RI.varifyID + 1;
                    }
                    else
                    {
                        NRI.varifyID = RI.varifyID;
                    }

                    __sync_fetch_and_add(&(pNode->rdRecord), 1);
                    UnLock(&pCacheTable->lock);
                    break;
                }
                UnLock(&pCacheTable->lock);

                //检测节点状态
                while (true)
                {
                    //检测节点是否过期
                    if (RI.index < pCacheTable->wIndex) 
                    {
                        if (RI.varifyID != pCacheTable->varifyID)
                        {
                            break; 
                        }

                        if (pNode->st == NODE_ST_RR || pNode->st == NODE_ST_NONE)
                        {
                            //节点可读 或者 节点失效
                            break;
                        }
                    }
                    else 
                    {
                        if (RI.index == pCacheTable->wIndex)
                        {
                            if (RI.varifyID != pCacheTable->varifyID && RI.varifyID  + 1 != pCacheTable->varifyID )
                            {
                                break;
                            }
                        }
                        else 
                        {
                            if (RI.varifyID  + 1 != pCacheTable->varifyID)
                            {
                                break; 
                            }

                            if (pNode->st == NODE_ST_RR || pNode->st == NODE_ST_NONE)
                            {
                                //节点可读 或者 节点失效
                                break;
                            }
                        }
                    }
 
                    if (DoSleep(spinInterval, leftTime) < 0)
                    {
                        RM_CBB_LOG_ERROR("PROQUICKTRANS","key[%02X] ID [%d]:: Read MSG Timeout[%d]\n", m_key, ID, timeout);
                        return -1;
                    }
                }
            }
            
            //数据长度
            if (NULL != outLen)
            {
                *outLen = pNode->len;
            }

            //读取数据
            if (data != NULL && sizeOfData >= pNode->len) 
            {
                ::memcpy(data, pNode->dataPtr, pNode->len);
                if(NULL != readIndex)
                {
                    ::memcpy(readIndex, &NRI, sizeof(ProQuickTransReadIndex_t));
                }
            }

            //设置读计数
            __sync_fetch_and_sub(&(pNode->rdRecord), 1);

            return 0;
        }

        /**
         * @brief                   检测节点中的操作是否完成 非进程安全
         *
         * @param checkNode         待检测的节点
         * @param sleepInterval     自旋间隔
         * @param timeout           超时时间
         *
         *
         * @return >= 0             操作均完成
         *         <  0             节点操作未完成
         */
        inline xint32_t DoNodeCheck(CacheTable_t * pCacheTable, Node_t * checkNode, xint32_t sleepInterval, xint32_t * timeout)
        {
            Node_t * nextNode = checkNode;
            
            if (NULL == nextNode || NULL == pCacheTable)
            {
                return 0;
            }

            while(checkNode->rdRecord > 0)
            {
                if (DoSleep(sleepInterval, timeout) < 0)
                {
                    RM_CBB_LOG_ERROR("PROQUICKTRANS"," DoCheck Timeout\n");
                    return -1;
                }
                continue;
            }

            if (checkNode->nextIndex != checkNode->checkNextIndex)
            {
                nextNode = (Node_t *)(pCacheTable->ptr + checkNode->checkNextIndex);

                while (true) 
                {
                    if (nextNode->st == NODE_ST_CHECK)
                    {
                        DoNodeCheck(pCacheTable, nextNode, sleepInterval, timeout);
                    }

                    if (nextNode->st == NODE_ST_WW || nextNode->rdRecord > 0)
                    {
                        if (DoSleep(sleepInterval, timeout) < 0)
                        {
                            RM_CBB_LOG_ERROR("PROQUICKTRANS"," DoCheck Timeout\n");
                            return -1;
                        }
                        continue;
                    }

                    if (nextNode->nextIndex == checkNode->nextIndex)
                    {
                        break;
                    }
                    
                    nextNode = (Node_t *)(pCacheTable->ptr + nextNode->nextIndex);
                }
            }

            checkNode->st = NODE_ST_NONE;
            return 0;
        }

        /**
         * @brief                   休眠并超时检测
         *
         * @param sleepInterval     休眠时间(ms) 0:使用yield切出线程
         * @param timeout           超时时间(ms) NULL:不做超时检测
         *
         * @return >= 0             未超时
         *                          超时
         */
        inline xint32_t DoSleep(xint32_t sleepInterval, xint32_t * timeout)
        {
            if (sleepInterval > 0)
            {
                usleep(sleepInterval * 1000);
                if(NULL != timeout)
                {
                    *timeout -= sleepInterval;
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
        xint32_t Init(key_t key, ProQuickTransCFG_t * cfgArray = NULL, xint32_t cfgNum = 0)
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
            if (NULL != cfgArray && cfgNum > 0)
            {
                m_shmID = ::shmget(m_key, totalSize, 0666 | IPC_CREAT | IPC_EXCL);
            }

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
                    //写锁
                    pthread_mutexattr_init(&table->wLockAttr);
                    pthread_mutexattr_setpshared(&table->wLockAttr, PTHREAD_PROCESS_SHARED);
                    pthread_mutex_init(&table->wLock, &table->wLockAttr);
 
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

                m_cachePool->st = 1;

                RM_CBB_LOG_WARNING("PROQUICKTRANS","ProQuickTrans Init key[%02X] (create) Success\n", m_key);
            }
            else
            {
                //共享缓存已存在 连接共享缓存
                if  ((m_shmID = ::shmget(m_key, 0, 0)) < 0)
                {
                    RM_CBB_LOG_ERROR("PROQUICKTRANS", "key [%02X] (connect) shmget Failed!\n", m_key);
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

            xint32_t needNodeCount = 0;

            xint32_t needLen = 0;

            xint32_t tmpLen = 0;

            xint32_t maxCacheSize = 0;

            xint32_t wIndex = 0;

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

            needLen = lenOfData + sizeof(Node_t);

            //获取写
            Lock(&pCacheTable->wLock);
            while (true)
            {
                wIndex = pCacheTable->wIndex;
                pNode = (Node_t *)(pCacheTable->ptr + wIndex);

                //占有块 
                tmpNode = pNode;
                tmpLen = 0;

                while(true)
                {
                    if (tmpNode->st == NODE_ST_CHECK)
                    {
                        //完成上一次写未完成工作
                        if (DoNodeCheck(pCacheTable, tmpNode,  spinInterval, leftTime) < 0)
                        {
                            RM_CBB_LOG_ERROR("PROQUICKTRANS","key[%02X] ID [%d] Error! Wait Node Check Timeout\n", m_key, ID);
                            UnLock(&pCacheTable->wLock);
                            return -2;
                        }
                    }

                    if (tmpNode->st == NODE_ST_WW)
                    {
                        if (DoSleep(spinInterval, leftTime) < 0)
                        {
                            RM_CBB_LOG_ERROR("PROQUICKTRANS","key[%02X] ID [%d]:: Write MSG Timeout[%d]\n", m_key, ID, timeout);
                            UnLock(&pCacheTable->wLock);
                            return -1;
                        }
                        //NODE_ST_WW 状态可能转为 NODE_ST_CHECK 所以需要再次检测一遍
                        continue;
                    }

                    tmpLen += tmpNode->size + sizeof(Node_t);
                    if (needLen <= tmpLen || tmpNode->nextIndex == 0)
                    {
                        //达到拼接条件
                        
                        //锁住读写操作 块合并
                        Lock(&pCacheTable->lock);
                        pNode->st = NODE_ST_WW; 
                        pNode->size = tmpLen - sizeof(Node_t);
                        pNode->len = tmpLen - sizeof(Node_t);
                        pNode->checkNextIndex = pNode->nextIndex;
                        if (tmpNode->nextIndex != 0)
                        {
                            pNode->nextIndex = wIndex + tmpLen;
                        }
                        else
                        {
                            pNode->nextIndex = 0;
                        }

                        tmpNode = (Node_t *)(pCacheTable->ptr + pNode->nextIndex);
                        tmpNode->preIndex = wIndex;
                        pCacheTable->wIndex = pNode->nextIndex;
                        if (0 == pCacheTable->wIndex)
                        {
                            pCacheTable->varifyID++;
                        }
                        //合并完成 - 放开读 
                        UnLock(&pCacheTable->lock);
                        
                        //放开写
                        UnLock(&pCacheTable->wLock);
                        break;
                    }

                    tmpNode = (Node_t *)(pCacheTable->ptr +  tmpNode->nextIndex);
                }

                //等待读操作完成 即可写操作
                while(pNode->rdRecord > 0)
                {
                    if ((DoSleep(spinInterval, leftTime)) < 0)
                    {
                        RM_CBB_LOG_ERROR("PROQUICKTRANS","key[%02X] ID [%d]:: Write MSG Timeout[%d]\n", m_key, ID, timeout);
                        //让下次写操作检测即可
                        pNode->st = NODE_ST_CHECK;
                        return -1;
                    }
                }

                if (pNode->nextIndex != pNode->checkNextIndex)
                {
                    tmpNode = (Node_t *)(pCacheTable->ptr + pNode->checkNextIndex);
                    while(true)
                    {
                        while (tmpNode->rdRecord > 0)
                        {
                            if ((DoSleep(spinInterval, leftTime)) < 0)
                            {
                                RM_CBB_LOG_ERROR("PROQUICKTRANS","key[%02X] ID [%d]:: Write MSG Timeout[%d]\n", m_key, ID, timeout);
                                //让下次写操作检测即可
                                pNode->st = NODE_ST_CHECK;
                                return -1;
                            }
                        }

                        if (tmpNode->nextIndex == pNode->nextIndex) 
                        {
                            break;
                        }

                        tmpNode = (Node_t *)(pCacheTable->ptr + tmpNode->nextIndex);
                    }
                }

                
                //检测占有的块是否能够写下数据
                if (pNode->size < lenOfData)
                {
                    //长度不够 继续占有块
                    pNode->len = pNode->size;
                    pNode->st = NODE_ST_NONE;
                    continue;
                }
                
                //写数据
                pNode->len = lenOfData;
                ::memcpy(pNode->dataPtr, data, lenOfData);

                //块拆分
                if (pNode->size - pNode->len > sizeof(Node_t)) 
                {
                    Node_t * nextNode = (Node_t *)(pCacheTable->ptr + pNode->nextIndex);

                    tmpNode = (Node_t *)(pCacheTable->ptr + wIndex + sizeof(Node_t) + pNode->len);
                    tmpNode->preIndex = wIndex;
                    tmpNode->nextIndex = pNode->nextIndex;
                    tmpNode->st = NODE_ST_NONE;
                    tmpNode->size = pNode->size - pNode->len - sizeof(Node_t);
                    tmpNode->len = tmpNode->size;
                    tmpNode->checkNextIndex = -1;
                    tmpNode->rdRecord = 0;

                    Lock(&pCacheTable->lock);
                    pNode->nextIndex = wIndex + sizeof(Node_t) + pNode->len;
                    pNode->size = pNode->len;

                    nextNode->preIndex = pNode->nextIndex;
                    UnLock(&pCacheTable->lock);

                }
                
                //设置可读
                pNode->st = NODE_ST_RR;

                //完成写
                break;
            }
            return 0;
        }

        /**
         * @brief                       读取数据
         *
         * @param ID                    类型ID
         * @param readIndex             读索引
         * @param data                  读数据缓存
         * @param sizeOfData            读数据缓存大小 
         * @param outLen                数据长度
         * @param mode                  读数据模式
         * @param timeout               超时时间 ms
         * @param spinInterval          自旋间隔 ms
         *
         * @return  >= 0                成功
         *          <  0                失败 
         */
        xint32_t Read(xint32_t ID, ProQuickTransReadIndex_t * readIndex, void * data, xint32_t sizeOfData, xint32_t * outLen, ProQuickTransReadMode_e mode = PRO_TRANS_READ_ORDER, xint32_t timeout = 0, xint32_t spinInterval = 1)
        {
            switch(mode)
            {
                case PRO_TRANS_READ_ORDER:
                    return Read_Order(ID, readIndex, data, sizeOfData, outLen, timeout , spinInterval);
                case PRO_TRANS_READ_OLDEST:
                    return Read_Order(ID, readIndex, data, sizeOfData, outLen, timeout , spinInterval);
                    break;
                case PRO_TRANS_READ_NEWEST:
                    return Read_Order(ID, readIndex, data, sizeOfData, outLen, timeout , spinInterval);
                    break;
                default:
                    RM_CBB_LOG_ERROR("PROQUICKTRANS","key[%02X] ID [%d] Error! read data Mode Error[%d]\n", m_key, ID, mode);
                    return -1;
            }
            return 0;
        }
    };
}
#endif
