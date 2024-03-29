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

namespace RM_CODE 
{
    class ProQuickTrans {
    public:
        //typedef struct _IndexNode {
        //    xint32_t index; 
        //    xint32_t nodeID;
        //    xint32_t ID;
        //    //0 顺序读 1 最旧值 2 最新值
        //    xint32_t mode; 
        //} IndexNode_t;

    private:
        typedef ProQuickTransNodeIndex_t IndexNode_t;
        typedef struct _CFG {
            xint32_t ID; 
            xint32_t blockCount; 
            xint32_t blockSize; 
        } CFG_t;


        typedef enum _NodeST {
            NODE_ST_WW,     //待写节点
            NODE_ST_RR,     //可读节点
            NODE_ST_F,      //空闲
            NODE_ST_NONE,   //无状态 标志位占用
        } NodeST_e;

        typedef struct _NodeInfo {
            xint32_t blockSize;
            xint32_t blockCount;
            xint32_t wIndex;
        } NodeInfo_t;

        typedef struct _Node {
            xint32_t nodeID;
            xint32_t rdRecord;
            NodeST_e st;
            xint32_t lenOfData;
            xint32_t block;
        } Node_t;

        typedef struct _TypeTable {
            pthread_mutex_t lock;     
            pthread_mutexattr_t lockAttr;
            NodeInfo_t nodeInfo;
            Node_t node[0];
        } TypeTable_t;
        
        typedef struct _CacheTable {
            int totalSize;              //map的大小
            int st;                     //map状态 0 未就绪 1 就绪
            int size;                   //数组typeTable的size
            //TypeTable_t ** typeTable;   //typeTable 数组
            int seekIndex[0];
            
        } CacheTable_t;


    private:
        int m_st;

        key_t m_key;

        std::map<xint32_t, CFG_t> m_cfg;

        pthread_mutex_t m_lock;
        
        CacheTable_t * m_cacheTable;

        xint32_t m_shmID;


    private:
        void Lock(pthread_mutex_t * lock)
        {
            ::pthread_mutex_lock(lock);
        }

        void UnLock(pthread_mutex_t * lock)
        {
            ::pthread_mutex_unlock(lock);
        }

        inline xint32_t NextIndex(xint32_t index, xint32_t size)
        {
            xint32_t next = index + 1;

            if (next < size)
            {
                return next;
            }
            else
            {
                return 0;
            }
        }

        inline xint32_t PreIndex(xint32_t index, xint32_t size)
        {
            if (index != 0)
            {
                return index - 1;
            }
            else
            {
                return size -1;
            }
        }


        xint32_t OrderRead(IndexNode_t * IN, xint32_t ID, xbyte_t * data, xint32_t sizeOfData, xint32_t interval = 10000, xint32_t timeout = 0)
        {
            TypeTable_t * typeTable = NULL;

            xint32_t isFirst = 1;

            xint32_t tag = 1;

            xint32_t len = 0;

            xint32_t retryTimes = timeout;

            if (NULL == IN || NULL == m_cacheTable || ID < 0 || ID >= m_cacheTable->size || m_cacheTable->seekIndex[ID] <= 0)
            {
                return -2;
            }

            xint32_t lastIndex = IN->index;
            xint32_t lastNodeID = IN->nodeID;

            xint32_t index = 0;

            typeTable = (TypeTable_t *)((xbyte_t *)m_cacheTable + m_cacheTable->seekIndex[ID]);


            while (tag == 1)
            {
                Lock(&(typeTable->lock));

                if (isFirst == 1)
                {
                    if (IN->index < 0 || IN->index >= typeTable->nodeInfo.blockCount || IN->nodeID != typeTable->node[IN->index].nodeID)
                    {
                        IN->index = NextIndex(typeTable->nodeInfo.wIndex, typeTable->nodeInfo.blockCount);
                        IN->nodeID = typeTable->node[IN->index].nodeID;
                    }
                    else
                    {

                        IN->index = NextIndex(IN->index, typeTable->nodeInfo.blockCount);
                        IN->nodeID = typeTable->node[IN->index].nodeID;
                    }
                    isFirst = 0;
                }
                else if (IN->nodeID != typeTable->node[IN->index].nodeID)
                {

                    IN->index = NextIndex(typeTable->nodeInfo.wIndex, typeTable->nodeInfo.blockCount);
                    IN->nodeID = typeTable->node[IN->index].nodeID;
                }



                if (typeTable->node[IN->index].st == NODE_ST_RR)
                {
                    __sync_fetch_and_add(&(typeTable->node[IN->index].rdRecord), 1);
                    //typeTable->node[IN->index].rdRecord++;
                    len = typeTable->node[IN->index].lenOfData;
                    tag = 0;
                }
                else if (typeTable->node[IN->index].st == NODE_ST_F)
                {
                    IN->index = 0;
                    IN->nodeID = typeTable->node[IN->index].nodeID;
                }

                index = IN->index;

                UnLock(&(typeTable->lock));
                while(tag == 1)
                {
                    if (typeTable->node[index].st == NODE_ST_RR)
                    {
                        break;
                    }

                    if (interval > 0)
                    {
                        ::usleep(interval);
                    }
                    else
                    {
                        ::sched_yield();
                    }

                    if (timeout > 0 && (retryTimes -= interval) <= 0)
                    {

                        IN->index = lastIndex;
                        IN->nodeID = lastNodeID;

                        return -2;
                    }

                }
            }

            if (sizeOfData >= typeTable->node[index].lenOfData)
            {
                ::memcpy(data, (xbyte_t *)m_cacheTable + typeTable->node[index].block,  typeTable->node[index].lenOfData);
            }
            else
            {

                IN->index = lastIndex;
                IN->nodeID = lastNodeID;
                len = -4;
            }
            __sync_fetch_and_sub(&(typeTable->node[index].rdRecord), 1);


            return len;
        }

        xint32_t ReadLast(IndexNode_t * IN, xint32_t ID, xbyte_t * data, xint32_t sizeOfData, xint32_t interval = 10000, xint32_t timeout = 0)
        {
            TypeTable_t * typeTable = NULL;

            xint32_t isFirst = 1;

            xint32_t tag = 1;

            xint32_t len = 0;

            xint32_t retryTimes = timeout;

            if (NULL == IN || NULL == m_cacheTable || ID < 0 || ID >= m_cacheTable->size || m_cacheTable->seekIndex[ID] <= 0)
            {
                return -2;
            }

            typeTable = (TypeTable_t *)((xbyte_t *)m_cacheTable + m_cacheTable->seekIndex[ID]);


            while (tag == 1)
            {
                Lock(&(typeTable->lock));

                if (isFirst == 1)
                {
                    IN->index = NextIndex(typeTable->nodeInfo.wIndex, typeTable->nodeInfo.blockCount);
                    IN->nodeID = typeTable->node[IN->index].nodeID;
                    isFirst = 0;
                }
                else if (IN->nodeID != typeTable->node[IN->index].nodeID)
                {
                    IN->index = NextIndex(typeTable->nodeInfo.wIndex, typeTable->nodeInfo.blockCount);
                    IN->nodeID = typeTable->node[IN->index].nodeID;
                }

                if (typeTable->node[IN->index].st == NODE_ST_RR)
                {
                    __sync_fetch_and_add(&(typeTable->node[IN->index].rdRecord), 1);
                    len = typeTable->node[IN->index].lenOfData;
                    tag = 0;
                }
                else if (typeTable->node[IN->index].st == NODE_ST_F)
                {
                    IN->index = 0;
                    IN->nodeID = typeTable->node[IN->index].nodeID;
                }
                UnLock(&(typeTable->lock));

                while(tag == 1)
                {
                    if (typeTable->node[IN->index].st == NODE_ST_RR)
                    {
                        break;
                    }

                    ::usleep(interval);

                    if ( timeout > 0 && (retryTimes -= interval ) <= 0)
                    {
                        return -2;
                    }
                }
            }

            if (sizeOfData >= typeTable->node[IN->index].lenOfData)
            {
                ::memcpy(data, (xbyte_t *)m_cacheTable + typeTable->node[IN->index].block,  typeTable->node[IN->index].lenOfData);
            }
            else
            {
                len = -4;
            }

            __sync_fetch_and_sub(&(typeTable->node[IN->index].rdRecord), 1);
            return len;
        }

        xint32_t ReadCurrent(IndexNode_t * IN, xint32_t ID, xbyte_t * data, xint32_t sizeOfData, xint32_t interval = 10000, xint32_t timeout = 0)
        {

            TypeTable_t * typeTable = NULL;

            xint32_t isFirst = 1;

            xint32_t tag = 1;

            xint32_t len = 0;

            xint32_t retryTimes = timeout;

            if (NULL == IN || NULL == m_cacheTable || ID < 0 || ID >= m_cacheTable->size || m_cacheTable->seekIndex[ID] <= 0)
            {
                return -2;
            }

            typeTable = (TypeTable_t *)((xbyte_t *)m_cacheTable + m_cacheTable->seekIndex[ID]);

            while (tag == 1)
            {
                Lock(&(typeTable->lock));

                if (isFirst == 1)
                {
                    IN->index = PreIndex(typeTable->nodeInfo.wIndex, typeTable->nodeInfo.blockCount);
                    IN->nodeID = typeTable->node[IN->index].nodeID;
                    isFirst = 0;
                }
                else if (IN->nodeID != typeTable->node[IN->index].nodeID)
                {

                    IN->index = PreIndex(typeTable->nodeInfo.wIndex, typeTable->nodeInfo.blockCount);
                    IN->nodeID = typeTable->node[IN->index].nodeID;
                }

                if (typeTable->node[IN->index].st == NODE_ST_RR)
                {
                    __sync_fetch_and_add(&(typeTable->node[IN->index].rdRecord), 1);
                    len = typeTable->node[IN->index].lenOfData;
                    tag = 0;
                }
                else if (typeTable->node[IN->index].st == NODE_ST_F)
                {
                    IN->index = 0;
                    IN->nodeID = typeTable->node[IN->index].nodeID;
                }
                UnLock(&(typeTable->lock));

                while(tag == 1)
                {
                    if (typeTable->node[IN->index].st == NODE_ST_RR)
                    {
                        break;
                    }

                    if (timeout > 0 && (retryTimes -= interval) <= 0)
                    {
                        return -2;
                    }
                }
            }

            if (sizeOfData >= typeTable->node[IN->index].lenOfData)
            {
                ::memcpy(data, (xbyte_t *)m_cacheTable + typeTable->node[IN->index].block,  typeTable->node[IN->index].lenOfData);
            }
            else
            {
                len = -4;
            }

            __sync_fetch_and_sub(&(typeTable->node[IN->index].rdRecord), 1);

            return len;
        }

    public:
        ProQuickTrans()
        {
            m_st = 0;
            m_shmID = -1;
            m_cacheTable = NULL;
            m_key = 0x00;
            pthread_mutex_init(&m_lock, NULL);
        }

        /**
         * @brief                   登记配置
         *
         * @param[in]ID             类型ID 建议值0开始累加且连续
         * @param[in]blockCount     缓存记录数条数
         * @param[in]blockSize      记录的大小
         *
         * @return  >= 0            成功 1 重读覆盖记录
         *          <  0            失败
         */
        xint32_t AddCFG(xint32_t ID, xint32_t blockCount, xint32_t blockSize)
        {
            CFG_t cfg = {0};

            if (ID < 0 || blockCount <= 0 || blockSize < 0)
            {
                return -1;
            }
            
            cfg.ID = ID;
            cfg.blockCount = blockCount + 1;
            cfg.blockSize = blockSize;
            ::pthread_mutex_lock(&m_lock);
            m_cfg[ID] = cfg;
            ::pthread_mutex_unlock(&m_lock);

            return 0;
        }


        xint32_t Init(key_t key)
        {
            xint32_t totalSize;

            xint32_t retryTimes = 5;

            xint32_t maxID = -1;

            xbyte_t * ptr = NULL;

            TypeTable_t * typeTable = NULL;

            std::map<xint32_t, CFG_t> tmpCFG;

            std::map<xint32_t, CFG_t>::iterator it;

            if (m_st == 1)
            {
                return 0;
            }
            
            m_key = key;
            ::pthread_mutex_lock(&m_lock);
            tmpCFG = m_cfg;
            ::pthread_mutex_unlock(&m_lock);

            totalSize = sizeof(CacheTable_t);

            for (it = tmpCFG.begin(); it != tmpCFG.end(); it++)
            {
                if (it->first > maxID)
                {
                    maxID = it->first;
                }

                totalSize += sizeof(TypeTable_t);
                totalSize += sizeof(Node_t) * it->second.blockCount;
                totalSize += it->second.blockCount * it->second.blockSize;
            }

            totalSize += (maxID + 1) * sizeof(TypeTable_t *);
            
            //创建共享缓存
            m_shmID = ::shmget(m_key, totalSize, 0666 | IPC_CREAT | IPC_EXCL);
            if (m_shmID >= 0)
            {
                //创建成功 初始化
                if ((m_cacheTable = (CacheTable_t *)::shmat(m_shmID, NULL, 0)) == NULL)
                {
                    printf("(Create Shm) shmat failed!%d\n", errno);
                    return -2;
                }
                
                ::memset(m_cacheTable, 0x00, totalSize);

                ptr = (xbyte_t *)m_cacheTable;

                m_cacheTable->st = 0;
                m_cacheTable->totalSize = totalSize;
                m_cacheTable->size = maxID + 1;

                ptr += sizeof(CacheTable_t);

                //m_cacheTable->seekIndex = (int *)ptr;

                ptr += (maxID + 1) * sizeof(int);


                for (it = tmpCFG.begin(); it != tmpCFG.end(); it++ )
                {
                    typeTable = (TypeTable_t *)ptr;
                    //m_cacheTable->typeTable[it->first] = typeTable;
                    m_cacheTable->seekIndex[it->first] = (int)(ptr - (xbyte_t *)m_cacheTable);
                    //进程锁初始化
                    pthread_mutexattr_init(&typeTable->lockAttr);

                    pthread_mutexattr_setpshared(&typeTable->lockAttr, PTHREAD_PROCESS_SHARED);

                    pthread_mutex_init(&typeTable->lock, &typeTable->lockAttr);
 
                    typeTable->nodeInfo.blockSize = it->second.blockSize;
                    typeTable->nodeInfo.blockCount = it->second.blockCount;
                    typeTable->nodeInfo.wIndex = 0;

                    ptr += sizeof(TypeTable_t);


                    ptr += it->second.blockCount * sizeof(Node_t);


                    for (xint32_t iLoop = 0; iLoop < it->second.blockCount; iLoop++)
                    {
                        typeTable->node[iLoop].st = NODE_ST_F;
                        typeTable->node[iLoop].rdRecord = 0;
                        typeTable->node[iLoop].nodeID = iLoop + 1;
                        typeTable->node[iLoop].block = (xint32_t)(ptr + iLoop * it->second.blockSize - (xbyte_t *)m_cacheTable);
                    }


                    ptr += it->second.blockCount * it->second.blockSize;
                }
                
                //就绪
                m_cacheTable->st = 1;
            }
            else
            {
                //创建失败 连接共享缓存 等待共享内存就绪
                if ((m_shmID = ::shmget(m_key, 0, 0)) < 0)
                {
                    printf("(Exist shm) ShmGet Failed! %d\n", errno);
                    return -3;
                }

                if ((m_cacheTable = (CacheTable_t *)::shmat(m_shmID, NULL, 0)) == NULL)
                {
                    printf("(Exist shm) Shmat Failed! %d\n", errno);
                    return -3;
                }

                while (retryTimes > 0)
                {
                    if (m_cacheTable->st == 1)
                    {
                        break; 
                    }
                    ::usleep(500000);
                    retryTimes--;
                    
                }

                if (retryTimes <= 0)
                {
                    printf("Shm Not Ready! Timeout!\n");
                    m_st = 0;
                    return -2;
                }
            }

            return 0; 
        }
        
        void UnInit()
        {
            ::pthread_mutex_lock(&m_lock);
            m_st = 0;
            if (m_cacheTable != NULL)
            {
                ::shmdt(m_cacheTable);
                m_cacheTable = NULL;
            }
            ::pthread_mutex_unlock(&m_lock);
        }


        xint32_t DataSize(xint32_t ID)
        {
            if (m_cacheTable == NULL || ID < 0 || ID >= m_cacheTable->size || m_cacheTable->seekIndex[ID] <= 0)
            {
                return -1;
            }

            TypeTable_t * typeTable = (TypeTable_t *)((xbyte_t *)m_cacheTable + m_cacheTable->seekIndex[ID]);
            
            return typeTable->nodeInfo.blockSize;
        }

        xint32_t Write(xint32_t ID, xbyte_t * data, xint32_t lenOfData, xint32_t interval = 10000, xint32_t timeout = 0)
        {
            TypeTable_t * typeTable = NULL;

            xint32_t retryTimes = timeout;

            xint32_t index = -1;

            xint32_t next = 0;

            NodeST_e st;

            if (m_cacheTable == NULL || ID < 0 || ID >= m_cacheTable->size || m_cacheTable->seekIndex[ID] <= 0)
            {
                return -1;
            }
            
            typeTable = (TypeTable_t *)((xbyte_t *)m_cacheTable + m_cacheTable->seekIndex[ID]);

            if (lenOfData > typeTable->nodeInfo.blockSize)
            {
                return -2;
            }

            index = -1;

            while (true)
            {

                index = typeTable->nodeInfo.wIndex;
                if (index < 0 || index >= typeTable->nodeInfo.blockSize || typeTable->node[index].st == NODE_ST_WW)
                {
                    if (interval > 0)
                    {
                        ::usleep(interval);
                    }
                    else
                    {
                        ::sched_yield();
                    }
                    if (timeout < 0 && (retryTimes -= (interval + 1)) == 0)
                    {
                        return -2;
                    }
                    continue;
                }

                //检测状态正常后进行锁 -- 减少锁的次数
                Lock(&typeTable->lock);
                if (typeTable->node[typeTable->nodeInfo.wIndex].st != NODE_ST_WW)
                {
                    st = typeTable->node[typeTable->nodeInfo.wIndex].st;
                    typeTable->node[typeTable->nodeInfo.wIndex].st = NODE_ST_WW;

                    index = typeTable->nodeInfo.wIndex;
                    typeTable->nodeInfo.wIndex = NextIndex(typeTable->nodeInfo.wIndex, typeTable->nodeInfo.blockCount);

                    typeTable->node[typeTable->nodeInfo.wIndex].st = NODE_ST_NONE;
                    typeTable->node[typeTable->nodeInfo.wIndex].nodeID++;
                    UnLock(&typeTable->lock);
                    break;
                }
                UnLock(&typeTable->lock);
            }     

            while (typeTable->node[index].rdRecord > 0)
            {
                //::sched_yield();
                usleep(0);
 
                if ((--retryTimes) == 0)
                {
                    //恢复状态
                    typeTable->node[index].st = st;
                    return -2;
                }
            }

            //数据拷贝
            typeTable->node[index].lenOfData = lenOfData;
            ::memcpy(typeTable->node[index].block + (xbyte_t *)m_cacheTable, data, lenOfData);
            //提交状态 开发读
            typeTable->node[index].st = NODE_ST_RR;

            return 0;
        }

        
        xint32_t Read(IndexNode_t * IN, xint32_t ID, xbyte_t * data, xint32_t sizeOfData, xint32_t interval = 10000, xint32_t timeout = 0)
        {
            if (NULL == IN || NULL == m_cacheTable || NULL == data)
            {
                return -1;
            }

            switch (IN->mode)
            {
                case 0:
                default:
                {
                    return OrderRead(IN, ID, data, sizeOfData, interval, timeout);
                }
                case 1:
                {
                    return ReadLast(IN, ID, data, sizeOfData, interval, timeout);
                }
                case 2:
                {
                    return ReadCurrent(IN, ID, data, sizeOfData, interval, timeout);
                }
            }
            //::memcpy(data, typeTable->node[IN->index].block,  typeTable->node[IN->index].lenOfData);
            //__sync_fetch_and_sub(&(typeTable->node[IN->index].rdRecord), 1);
        }
    };
}
#endif
