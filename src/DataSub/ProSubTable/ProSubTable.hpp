#ifndef __LYW_CODE_PRO_SUB_TABLE_H_FILE__
#define __LYW_CODE_PRO_SUB_TABLE_H_FILE__
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

namespace LYW_CODE
{
    class ProSubTable
    {
    private:
#pragma pack(1)
        typedef struct _MapInfo
        {
            //引用计数 0 无连接
            int reCount;
            
            //ProMap_t size 为 8 的整数倍 整数字节计较方便 初始为 32 每次递增 32 
            int proMapSize;
            
            //MsgMap_t size 初始为 1024 每次递增 1024
            int msgMapSize;

            //MsgMap_t::proSubMap size 为 proMapSize / 8
            int proSubMapSize;

        } MapInfo_t;

        /**
         * @brief       进程注册表 结构为数组 固定长度 超车扩容 进程索引为数组索引
         *
         */
        typedef struct _ProMap
        {
            pid_t pid;

            // 0 无效 1 有效
            int st;
            
            //快速获取 索引
            int proIndex;

        } ProMap_t;
        
        /**
         * @brief       消息注册表 键值为 消息索引 值为消息详情 包含消息id 订阅该消息的进程bitMap 
         *
         *
         */
        typedef struct _MsgMap
        {
            int msgID;

            int msgIndex;

            // 0 无效 1 有效
            int st;
            
            //订阅该消息的进程 bitMap 索引为 proIndex  0 未的订阅 1 订阅
            unsigned char proSubMap[0];

        } MsgMap_t;

        typedef struct _ProSubTable
        {
            unsigned int ready;                         //为 2B2B 时就绪 否则未就绪
            pthread_mutex_t lock;                       //用于同步状态切换操作
            pthread_mutexattr_t lockAttr;           

            key_t mapKey;                               //注册表共享内存键值 扩容后 mapKey会变更 需要重新连接并同步数据

        } ProSubTable_t;
#pragma pack()

    private:
        //subTableKey
        key_t m_key;

        int m_tableShmID;

        ProSubTable_t * m_proSubTable;
        
        // 0 未连接 1 连接 
        int m_st;

        pthread_mutex_t m_lock;
        
        //与 m_proSubTable 匹配 则说明 map 未发生变更 否则 map变更需要本地同步

        //本地缓存 - 防止写共享缓存 导致脏数据  
        MapInfo_t * m_mapInfo;

        ProMap_t * m_proMap;

        MsgMap_t * m_msgMap;

        unsigned int m_reCount;

        MapInfo_t * m_mapInfo_swap;

        ProMap_t * m_proMap_swap;

        MsgMap_t * m_msgMap_swap;

        unsigned int m_reCount_swap;


        key_t m_mapKey;

        
        // 0 更新结束 1 更新中
        int m_mapUpdateTag;


    private:
        
        /**
         * @brief               bitMap 设置bit位未 0 或 1 为减少检测次数 请自行保证 index 的合法性 崩溃概不负责
         *
         * @param[in]bitMap     bitMap 不做长度检测
         * @param[in]index      索引
         * @param[in]setValue   0 设置未 0  否则则设置为 1
         */
        inline void SetBit(unsigned char * bitMap, int index, unsigned char setValue)
        {
            unsigned char bit;
            unsigned int byteIndex = index / 8;
            unsigned int bitIndex = 7 - (index - byteIndex * 8);

            bit = 0x01;
            bit = bit << bitIndex;

            if (setValue == 0x00)
            {
                bitMap[byteIndex] &= (~bit);

            }
            else
            {
                bitMap[byteIndex] |= bit;
            }
        }
        
        /**
         * @brief               获取bitMap 索引下 bit位值 为减少检测次数 请自行保证 index 的合法性 崩溃概不负责
         * @param[in]bitMap     bitMap 不做长度检测
         * @param[in]index      索引
         *
         * @return      0       索引位为 0 
         *              1       索引位位 1
         */
        inline int GetBit(unsigned char * bitMap, int index)
        {
            unsigned char bit = 0x01;
            unsigned int byteIndex = index / 8;
            unsigned int bitIndex = 7 - (index - byteIndex * 8);
            bit = bit << bitIndex;
            
            if ((bitMap[byteIndex] & bit) == 0)
            {
                return 0;
            }
            else
            {
                return 1;
            }
        }


        /**
         * @brief                       创建订阅map，请保证 proSubTable 已完成创建后使用 非线程安全
         *
         * @param[in] mapInfo           map尺寸信息
         *
         * @return  >= 0                成功
         *          <  0                失败 错误码
         */
        int ExpandMap(const MapInfo_t * pMapInfo)
        {
            int len = 0;

            int proMapSize = 0;

            int msgMapSize = 0;

            int proSubMapSize = 0;

            int min = 0;

            key_t key;

            int shmID = 0;

            int oldShmID = 0;

            int retry = 3;

            unsigned char * ptr;

            MapInfo_t * pOldMapInfo = NULL;

            MapInfo_t * pNewMapInfo = NULL;

            if (m_proSubTable == NULL || pMapInfo == NULL )
            {
                return -1;
            }

            if (pMapInfo->proMapSize % 8 != 0 || pMapInfo->proMapSize / 8 != pMapInfo->proSubMapSize)
            {
                return -2;
            }

            //创建共享缓存
            len = sizeof(MapInfo_t) + sizeof(ProMap_t) * pMapInfo->proMapSize + (sizeof(MsgMap_t) + pMapInfo->proSubMapSize) * pMapInfo->msgMapSize;
            
            std::srand(time(NULL));

            while(retry > 0)
            {
                key = std::rand();
                shmID = ::shmget(key, len, 0666 | IPC_CREAT | IPC_EXCL);
                if (shmID > 0)
                {
                    if ((pNewMapInfo = (MapInfo_t *)::shmat(shmID, NULL, 0)) != NULL)
                    {
                        //连接成功 跳出循环 进行初始化
                        break;
                    }
                    else
                    {
                        //连接失败 重新创建
                        Destroy(key);
                    }
                }
                retry--;
            }
            
            if (retry <= 0)
            {
                return -3;
            }
            
            ::memset(pNewMapInfo, 0x00, len);

            //初始化 -- 不存在旧值拷贝问题
            pNewMapInfo->reCount = 0;
            pNewMapInfo->proMapSize = pMapInfo->proMapSize;
            pNewMapInfo->msgMapSize = pMapInfo->msgMapSize;
            pNewMapInfo->proSubMapSize = pMapInfo->proSubMapSize;

            //是否存在旧map
            if (m_proSubTable->mapKey != 0x00)
            {
                //已初始化 -- 需复制旧map值至新map
                if ((oldShmID = ::shmget(m_proSubTable->mapKey, 0, 0)) >= 0)
                {
                    //连接至当前MAP
                    if ((pOldMapInfo = (MapInfo_t *)::shmat(oldShmID, NULL, 0)) != NULL)
                    {
                        ProMap_t * newProMap = (ProMap_t *)(((unsigned char *)pNewMapInfo) + sizeof(MapInfo_t)) ;
                        ProMap_t * oldProMap = (ProMap_t *)(((unsigned char *)pOldMapInfo) + sizeof(MapInfo_t)) ;

                        MsgMap_t * newMsgMap = (MsgMap_t *)(((unsigned char *)pNewMapInfo) + sizeof(MapInfo_t) + pNewMapInfo->proMapSize * sizeof(ProMap_t));

                        MsgMap_t * oldMsgMap = (MsgMap_t *)(((unsigned char *)pOldMapInfo) + sizeof(MapInfo_t) + pOldMapInfo->proMapSize * sizeof(ProMap_t));


                        //拷贝 proMap
                        if (pOldMapInfo->proMapSize > pNewMapInfo->proMapSize) 
                        {
                            min = pNewMapInfo->proMapSize;
                        }
                        else
                        {
                            min = pOldMapInfo->proMapSize;
                        }

                        ::memcpy(newProMap, oldProMap, min * sizeof(ProMap_t));

                        //拷贝 MsgMap
                        if (pOldMapInfo->proSubMapSize > pNewMapInfo->proSubMapSize) 
                        {
                            min = pNewMapInfo->proSubMapSize;
                        }
                        else
                        {
                            min = pOldMapInfo->proSubMapSize;
                        }

                        for (int iLoop = 0; iLoop < pNewMapInfo->msgMapSize && iLoop < pOldMapInfo->msgMapSize; iLoop++)
                        {
                            ::memcpy(newMsgMap, oldMsgMap, sizeof(MsgMap_t) + min);
                            newMsgMap = (MsgMap_t * )(((unsigned char *)newMsgMap) + sizeof(MsgMap_t) + pNewMapInfo->proSubMapSize);
                            oldMsgMap = (MsgMap_t * )(((unsigned char *)oldMsgMap) + sizeof(MsgMap_t) + pOldMapInfo->proSubMapSize);

                        }
                        
                        //断开连接 oldmap
                        ::shmdt(pOldMapInfo);
                    }
                }
            }
            
            //断开新建map
            ::shmdt(pNewMapInfo);

            //删除旧MAP -- 共享缓存存在引用机制 保证释放安全
            if (m_proSubTable->mapKey != 0x00)
            {
                Destroy(m_proSubTable->mapKey);
            }
            
            //提交新Map 只proSubTable 并更新版本
            m_proSubTable->mapKey = key;

            return 0;
        }
        
        /**
         * @brief               将map更新至本地 非进程安全 本地线程安全 
         *
         * return   >= 0        成功 
         *          <  0        失败
         */
        int ConnectMap()
        {
            int shmID;

            MapInfo_t * pMapInfo = NULL;

            MapInfo_t * pOldMapInfo = NULL;

            int reCount = 0;


            if (m_proSubTable == NULL && m_proSubTable->mapKey != 0x00)
            {
                return -1;
            }


            if (m_mapKey ==  m_proSubTable->mapKey)
            {
                return 0;
            }

            if ((shmID = ::shmget(m_proSubTable->mapKey, 0, 0)) < 0)
            {
                return -2;
            }

            if ((pMapInfo = (MapInfo_t *)::shmat(shmID, NULL, 0)) == NULL)
            {
                return -3;
            }
            
            
            //同步map 至本地 本地资源需要锁 进程已锁 写能够同步
            //pthread_mutex_lock(&m_lock);
            
            //旧值拷贝值交换区
            m_mapInfo_swap = m_mapInfo;
            m_proMap_swap = m_proMap;
            m_msgMap_swap = m_msgMap;

            reCount = m_reCount;
            m_mapUpdateTag = 1;

            //自旋等待 旧数据读取操作完成 由于map切换操作属于 读频繁 写极少出现 使用该策略 写时会导致部分读延迟 保证读不进行任何锁操作
            while(true) 
            {
                ::usleep(10000);
                if (reCount == m_reCount)
                {
                    break;
                }
                else
                {
                    reCount = m_reCount;
                }
            }

            //更新新指针
            pOldMapInfo = m_mapInfo;
            m_mapKey = m_proSubTable->mapKey;
            m_mapInfo = pMapInfo;
            m_proMap = (ProMap_t *)((unsigned char *)m_mapInfo + sizeof(MapInfo_t));
            m_msgMap = (MsgMap_t *)((unsigned char *)m_proMap + sizeof(ProMap_t) * m_mapInfo->proMapSize);
            
            //放开读操作
            reCount = m_reCount_swap;
            m_mapUpdateTag = 0;
            //自旋等待 交换区内的读操作完成 10ms 为经验值 应大于 一次读操作的完成时间 才能保证安全
            while(true) 
            {
                ::usleep(10000);
                if (reCount == m_reCount)
                {
                    break;
                }
                else
                {
                    reCount = m_reCount;
                }
            }
            //pthread_mutex_unlock(&m_lock);

            //断开内存连接
            if (pOldMapInfo != NULL)
            {
                ::shmdt(pOldMapInfo);
            }

            return 0;
        }

        /**
         * @brief               检测是否初始化完成 并连接相关资源 存在同步 禁止锁
         *
         * @return      >=0     成功 
         *              < 0     失败 不许进行业务操作
         */
        int IsInit()
        {
            int ret = 0;
            //table 未完成初始化
            if (m_st != 1 || m_proSubTable == NULL) 
            {
                return -1;
            }
            
            //map 是否需要重新连接
            if (m_mapInfo == NULL || m_mapKey != m_proSubTable->mapKey)
            {
                ::pthread_mutex_lock(&m_proSubTable->lock);
                ret = ConnectMap();
                ::pthread_mutex_unlock(&m_proSubTable->lock);
                
                if (ret < 0)
                {
                    return -2;
                }
            }
            return 0;
        }

    public:
        ProSubTable(key_t key)
        {
            m_key = key;

            m_st = 0;

            m_proSubTable = NULL;

            m_mapInfo = NULL;

            m_proMap = NULL;

            m_msgMap = NULL;

            m_tableShmID = -1;

            pthread_mutex_init(&m_lock, NULL);

            m_mapUpdateTag = 0;

            m_reCount = 0;
        }

        int Create()
        {
            int len = sizeof(ProSubTable_t);

            MapInfo_t mapInfo;

            m_tableShmID = ::shmget(m_key, len, 0666 | IPC_CREAT | IPC_EXCL);

            if (m_tableShmID > 0)
            {
                m_proSubTable = (ProSubTable_t *)::shmat(m_tableShmID, NULL, 0);
                if (m_proSubTable == NULL)
                {
                    //create failed
                    return -1;
                }
                
                //初始化资源
                ::memset(m_proSubTable, 0x00, len);

                //mutex
                pthread_mutexattr_init(&m_proSubTable->lockAttr);
                //进程共享
                pthread_mutexattr_setpshared(&m_proSubTable->lockAttr, PTHREAD_PROCESS_SHARED);
                pthread_mutex_init(&m_proSubTable->lock, &m_proSubTable->lockAttr);

                m_proSubTable->mapKey = 0x00;

                
                //创建map
                mapInfo.proMapSize = 32;
                mapInfo.msgMapSize = 1024;
                mapInfo.proSubMapSize = 32 / 8;

                if (ExpandMap(&mapInfo) < 0)
                {
                    ::shmdt(m_proSubTable);
                    m_proSubTable = NULL;
                    m_tableShmID = -1;
                    Destroy(m_key);
                    return -1;
                }

                m_proSubTable->ready = 0x2B2B;

                //断开连接
                ::shmdt(m_proSubTable);
                m_proSubTable = NULL;
                m_tableShmID = -1;

                return 0;
            }

            return -1;
        }

        int Destroy()
        {
            int shmID = 0;

            ProSubTable_t * pTable;

            pthread_mutex_lock(&m_lock);

            if ((shmID = ::shmget(m_key, 0, 0)) < 0)
            {
                pthread_mutex_unlock(&m_lock);
                return 0;
            }

            if ((pTable = (ProSubTable_t *)::shmat(shmID, NULL, 0)) == NULL)
            {
                pthread_mutex_unlock(&m_lock);
                return 0;
            }

            Destroy(pTable->mapKey);

            ::shmdt(pTable);

            Destroy(m_key);

            if (m_mapInfo != NULL)
            {
                ::shmdt(m_mapInfo);
                m_mapKey = 0x00;
                m_mapInfo = NULL;
            }
            pthread_mutex_unlock(&m_lock);

            return 0;
        }

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

        int Connect()
        {
            int retry = 3;

            MapInfo_t * pMapInfo = NULL;

            int len = 0;

            int shmID = -1;

            key_t key;

            ::pthread_mutex_lock(&m_lock);
            if (m_st == 0)
            {
                if ((m_tableShmID = ::shmget(m_key, 0, 0)) < 0)
                {
                    ::pthread_mutex_unlock(&m_lock);
                    return -1;
                }

                if ((m_proSubTable = (ProSubTable_t *)::shmat(m_tableShmID, NULL, 0)) == NULL)
                {
                    ::pthread_mutex_unlock(&m_lock);
                    return -1;
                }

                //等待就绪
                while (retry > 0) 
                {
                    if (m_proSubTable->ready == 0x2B2B)
                    {
                        break;
                    }
                    else
                    {
                        ::usleep(500000);
                        retry--;
                    }
                }

                if (retry <= 0)
                {
                    ::pthread_mutex_unlock(&m_lock);
                    return -2;
                }

                m_st = 2;
                ::pthread_mutex_unlock(&m_lock);

                //同步Map
                ::pthread_mutex_lock(&m_proSubTable->lock);
                ConnectMap();
                ::pthread_mutex_unlock(&m_proSubTable->lock);
                m_st = 1;
                return 0;
            }
            ::pthread_mutex_unlock(&m_lock);
            return 0;
        }

        int DisConnect()
        {
            ::pthread_mutex_lock(&m_lock);
            m_st = 0;
            if (m_proSubTable != NULL)
            {
                ::shmdt(m_proSubTable);
                m_proSubTable = NULL;
            }

            if (m_mapInfo != NULL)
            {
                ::shmdt(m_mapInfo);
                m_mapKey = 0x00;
                m_mapInfo = NULL;
            }

            ::pthread_mutex_unlock(&m_lock);

            return 0;
        }
        
        /**
         * @brief                   进程注册
         *
         * @param[in]pro            待注册的进程ID
         * 
         * @return      >= 0        成功 进程注册ID
         *              <  0        失败 错误码
         */
        int ProRegister(pid_t pid = 0)
        {
            int ret = 0;

            int shmID;

            MapInfo_t * pMapInfo = NULL;

            ProMap_t * pProMap = NULL;

            ProMap_t * pFreeProMap = NULL;

            int proIndex = 0;

            int len = 0;

            //table 未完成初始化
            if (m_st != 1 || m_proSubTable == NULL) 
            {
                return -1;
            }
            
            ::pthread_mutex_lock(&m_proSubTable->lock);

            if (m_proSubTable == NULL && m_proSubTable->mapKey != 0x00)
            {
                ::pthread_mutex_unlock(&m_proSubTable->lock);
                return -1;
            }

            while(true)
            {

                if ((shmID = ::shmget(m_proSubTable->mapKey, 0, 0)) < 0)
                {
                    ::pthread_mutex_unlock(&m_proSubTable->lock);
                    return -2;
                }

                if ((pMapInfo = (MapInfo_t *)::shmat(shmID, NULL, 0)) == NULL)
                {
                    ::pthread_mutex_unlock(&m_proSubTable->lock);
                    return -3;
                }
                
                pProMap = (ProMap_t *)((unsigned char *)pMapInfo  + sizeof(MapInfo_t));
            
                for(int iLoop = 0; iLoop < pMapInfo->proMapSize; iLoop++)
                {
                    if (pProMap[iLoop].st == 0)
                    {
                        if (pFreeProMap == NULL)
                        {
                            pFreeProMap = &(pProMap[iLoop]);
                            pFreeProMap->proIndex = iLoop;
                        }
                    }
                    else
                    {
                        if (pProMap[iLoop].pid == pid)
                        {
                            proIndex = pProMap[iLoop].proIndex;
                            ::pthread_mutex_unlock(&m_proSubTable->lock);
                            ::shmdt(pMapInfo);
                            return proIndex;
                        }
                    }
                }

                if (pFreeProMap != NULL)
                {
                    pFreeProMap->pid = pid;
                    pFreeProMap->st = 1;
                    proIndex = pFreeProMap->proIndex;
                    ::pthread_mutex_unlock(&m_proSubTable->lock);
                    ::shmdt(pMapInfo);
                    return proIndex;
                }
                else
                {
                    MapInfo_t mapInfo;
                    mapInfo.proMapSize = pMapInfo->proMapSize + 32;
                    mapInfo.msgMapSize = pMapInfo->msgMapSize;
                    mapInfo.proSubMapSize = mapInfo.proMapSize / 8;
                    ::shmdt(pMapInfo);
                    ExpandMap(&mapInfo);
                }
            }
            ::pthread_mutex_unlock(&m_proSubTable->lock);

            return -2;
        }






        
        /**
         * @brief                   注册消息 需提供进程索引 与进程ID 注册过程会检测 proIndex 的合法性 
         *
         * @param[in]msgID          待注册的消息唯一id
         * @param[in]proIndex       进程注册后返回的进程索引
         * @param[in]pid            进程ID 进程唯一标识
         *
         * @return      >= 0        消息索引(句柄)
         *              <  0        错误码
         */
        int MsgRegister(int msgID, int proIndex, pid_t pid)
        {
            int ret = 0;

            int shmID;

            MapInfo_t * pMapInfo = NULL;

            ProMap_t * pProMap = NULL;

            MsgMap_t * pMsgMap = NULL;

            MsgMap_t * pFreeMsgMap = NULL;

            int msgIndex = 0;

            int len = 0;

            //table 未完成初始化
            if (m_st != 1 || m_proSubTable == NULL) 
            {
                return -1;
            }
            
            ::pthread_mutex_lock(&m_proSubTable->lock);

            if (m_proSubTable == NULL && m_proSubTable->mapKey != 0x00)
            {
                ::pthread_mutex_unlock(&m_proSubTable->lock);
                return -1;
            }

            while(true)
            {

                if ((shmID = ::shmget(m_proSubTable->mapKey, 0, 0)) < 0)
                {
                    ::pthread_mutex_unlock(&m_proSubTable->lock);
                    return -2;
                }

                if ((pMapInfo = (MapInfo_t *)::shmat(shmID, NULL, 0)) == NULL)
                {
                    ::pthread_mutex_unlock(&m_proSubTable->lock);
                    return -3;
                }
                
                pProMap = (ProMap_t *)((unsigned char *)pMapInfo  + sizeof(MapInfo_t));


                // 查看进程信息是否正确 
                if (pProMap[proIndex].st != 1 || pProMap[proIndex].pid != pid)
                {
                    ::pthread_mutex_unlock(&m_proSubTable->lock);
                    return -4;
                }

                // 查找消息是否注册 否则则添加消息
                pMsgMap = (MsgMap_t *)((unsigned char *)pProMap + sizeof(ProMap_t) * pMapInfo->proMapSize);

            
                for(int iLoop = 0; iLoop < pMapInfo->msgMapSize; iLoop++)
                {
                    if (pMsgMap->st == 0)
                    {
                        if (pFreeMsgMap == NULL)
                        {
                            pFreeMsgMap = pMsgMap;
                            pFreeMsgMap->msgIndex = iLoop;
                        }
                    }
                    else
                    {
                        if (pMsgMap->msgID == msgID)
                        {
                            msgIndex = pMsgMap->msgIndex;
                            SetBit(pMsgMap->proSubMap, proIndex, 1);
                            ::pthread_mutex_unlock(&m_proSubTable->lock);
                            ::shmdt(pMapInfo);
                            return msgIndex;
                        }
                    }

                    pMsgMap = (MsgMap_t *)((unsigned char *)pMsgMap + sizeof(MsgMap_t) + pMapInfo->proSubMapSize);
                }

                if (pFreeMsgMap != NULL)
                {
                    pFreeMsgMap->msgID = msgID;
                    pFreeMsgMap->st = 1;
                    
                    SetBit(pFreeMsgMap->proSubMap, proIndex, 1);

                    msgIndex = pFreeMsgMap->msgIndex;
                    ::pthread_mutex_unlock(&m_proSubTable->lock);
                    ::shmdt(pMapInfo);
                    return msgIndex;
                }
                else
                {
                    //msgMap 扩表
                    MapInfo_t mapInfo;
                    mapInfo.proMapSize = pMapInfo->proMapSize;
                    mapInfo.msgMapSize = pMapInfo->msgMapSize + 1024;
                    mapInfo.proSubMapSize = mapInfo.proMapSize / 8;
                    ::shmdt(pMapInfo);
                    ExpandMap(&mapInfo);
                }
            }
            ::pthread_mutex_unlock(&m_proSubTable->lock);
            return -2;
        }


        int ProUnRegister(int proIndex, pid_t pid)
        {
            int ret = 0;

            int shmID;

            int tag = 0;

            MapInfo_t * pMapInfo = NULL;

            ProMap_t * pProMap = NULL;

            int len = 0;

            //table 未完成初始化
            if (m_st != 1 || m_proSubTable == NULL) 
            {
                return -1;
            }
            
            ::pthread_mutex_lock(&m_proSubTable->lock);


            if (m_proSubTable == NULL && m_proSubTable->mapKey != 0x00)
            {
                ::pthread_mutex_unlock(&m_proSubTable->lock);
                return -1;
            }

            if ((shmID = ::shmget(m_proSubTable->mapKey, 0, 0)) < 0)
            {
                ::pthread_mutex_unlock(&m_proSubTable->lock);
                return -2;
            }

            if ((pMapInfo = (MapInfo_t *)::shmat(shmID, NULL, 0)) == NULL)
            {
                ::pthread_mutex_unlock(&m_proSubTable->lock);
                return -3;
            }

            if (proIndex >=  pMapInfo->proMapSize) 
            {
                ::pthread_mutex_unlock(&m_proSubTable->lock);
                return -4;
            }
            
            pProMap = (ProMap_t *)((unsigned char *)pMapInfo  + sizeof(MapInfo_t));

            
            if ((pid == 0 || pProMap[proIndex].pid == pid) && pProMap[proIndex].st == 1)
            {
                pProMap[proIndex].st = 0;
            }
            else
            {
                //信息不匹配
                ::pthread_mutex_unlock(&m_proSubTable->lock);
                return -5;
            }

            //注销进程订阅的所有消息
            MsgMap_t * pMsgMap = (MsgMap_t *)((unsigned char *)pProMap + sizeof(ProMap_t) * pMapInfo->proMapSize);

            for (int iLoop = 0; iLoop < pMapInfo->msgMapSize; iLoop++)
            {
                if (pMsgMap->st == 1)
                {
                    SetBit(pMsgMap->proSubMap, proIndex, 0);

                    tag = 0;

                    for (int iLoop1 = 0; iLoop1 < pMapInfo->proSubMapSize; iLoop1++ )    
                    {
                        if (pMsgMap->proSubMap[iLoop1] != 0x00)
                        {
                            //存在进程订阅
                            tag = 1;
                            break;
                        }
                    }

                    if (tag == 0)
                    {
                        //注销掉消息
                        pMsgMap->st = 0;
                    }
                }
            }

            //可以做缩表处理 暂时懒得写
            ::pthread_mutex_unlock(&m_proSubTable->lock);

            return 0;
        }

        void printfPro()
        {
            if (IsInit() < 0)
            {
                return;
            }

            for(int iLoop = 0; iLoop < m_mapInfo->proMapSize; iLoop++)
            {
                printf("proIndex %d  pid %d st %d\n", iLoop, m_proMap[iLoop].pid, m_proMap[iLoop].st);

            }
        }


        void printfMsg()
        {
            if (IsInit() < 0)
            {
                return;
            }
            
            MsgMap_t * msgMap = m_msgMap;

            for(int iLoop = 0; iLoop < m_mapInfo->msgMapSize; iLoop++)
            {
                printf("msgIndex %d  msgID %d st %d proSub %02X %02X\n", iLoop, msgMap->msgID, msgMap->st, msgMap->proSubMap[0], msgMap->proSubMap[1]);
                msgMap = (MsgMap_t *)((unsigned char *)msgMap + sizeof(MsgMap_t) + m_mapInfo->proSubMapSize);

            }
        }
 
        int MsgUnRegister(int msgIndex, int msgID, int proIndex, pid_t pid)
        {
            int ret = 0;

            int shmID;

            MapInfo_t * pMapInfo = NULL;

            ProMap_t * pProMap = NULL;

            MsgMap_t * pMsgMap = NULL;

            MsgMap_t * pFreeMsgMap = NULL;

            int len = 0;

            //table 未完成初始化
            if (m_st != 1 || m_proSubTable == NULL) 
            {
                return -1;
            }
            
            ::pthread_mutex_lock(&m_proSubTable->lock);

            if (m_proSubTable == NULL && m_proSubTable->mapKey != 0x00)
            {
                ::pthread_mutex_unlock(&m_proSubTable->lock);
                return -1;
            }


            if ((shmID = ::shmget(m_proSubTable->mapKey, 0, 0)) < 0)
            {
                ::pthread_mutex_unlock(&m_proSubTable->lock);
                return -2;
            }

            if ((pMapInfo = (MapInfo_t *)::shmat(shmID, NULL, 0)) == NULL)
            {
                ::pthread_mutex_unlock(&m_proSubTable->lock);
                return -3;
            }

            if (proIndex >= pMapInfo->proMapSize)
            {
                ::pthread_mutex_unlock(&m_proSubTable->lock);
                return -4;
            }

            if (msgIndex >= pMapInfo->msgMapSize)
            {
                ::pthread_mutex_unlock(&m_proSubTable->lock);
                return -5;
            }

            pProMap = (ProMap_t *)((unsigned char *)pMapInfo  + sizeof(MapInfo_t));

            // 查看进程信息是否正确 
            if (pProMap[proIndex].st != 1 || pProMap[proIndex].pid != pid)
            {
                ::pthread_mutex_unlock(&m_proSubTable->lock);
                return -4;
            }

            // 查看消息
            pMsgMap = (MsgMap_t *)((unsigned char *)pProMap + sizeof(ProMap_t) * pMapInfo->proMapSize + msgIndex * (sizeof(MsgMap_t) + pMapInfo->proSubMapSize));
            
            if (pMsgMap->st == 1 && pMsgMap->msgID == msgID)
            {
                SetBit(pMsgMap->proSubMap, proIndex, 0);
            }

            int tag = 0;

            for (int iLoop1 = 0; iLoop1 < pMapInfo->proSubMapSize; iLoop1++ )    
            {
                if (pMsgMap->proSubMap[iLoop1] != 0x00)
                {
                    //存在进程订阅
                    tag = 1;
                    break;
                }
            }

            if (tag == 0)
            {
                //注销掉消息
                pMsgMap->st = 0;
            }

            ::pthread_mutex_unlock(&m_proSubTable->lock);

            return 0;
        }


        /**
         * @brief               查询消息被订阅的信息
         *
         * @param[in]msgIndex   消息索引
         * @param[in]msgID      消息ID
         * @param[out]subMap    订阅表 为NULL时 获取长度
         * @param[in]sizeOfMap  map的size   
         * @param[out]lenOfMap  查询到map的len
         *
         * @return  >= 0        成功
         *          <  0        失败 -2 长度不足 使用lenOfMap 申请资源后再次获取即可
         *
         */
        int QuerySubPro(int msgIndex, int msgID, void * subMap, int sizeOfMap, int & lenOfMap)
        {
            MapInfo_t * mapInfo = NULL;

            ProMap_t * proMap = NULL;

            MsgMap_t * msgMap = NULL;
            
            if (IsInit() < 0)
            {
                return -1;
            }
            

            if (m_mapUpdateTag == 1)
            {
                //等待 更新完成 
                m_reCount_swap++;
                //进行读操作 使用 m_mapInfo_swap 此处代码禁止阻塞 不懂勿动
                mapInfo = m_mapInfo_swap;
                msgMap = m_msgMap_swap;
            }
            else
            {
                m_reCount++;
                //进行读操作 使用 m_mapInfo 此处代码禁止阻塞 不懂勿动
                mapInfo = m_mapInfo;
                msgMap = m_msgMap;
            }

            lenOfMap = mapInfo->proSubMapSize;
            
            if (subMap == NULL)
            {
                return 0;
            }

            if (sizeOfMap < mapInfo->proSubMapSize)
            {
                return -2;
            }

            if (msgIndex >= mapInfo->msgMapSize)
            {
                return -3;
            }

            msgMap = (MsgMap_t *)((unsigned char *)msgMap + (sizeof(MsgMap_t) + mapInfo->proSubMapSize) * msgIndex);
            
            if (msgMap->st == 1 && msgMap->msgID == msgID)
            {
                ::memcpy(subMap, msgMap->proSubMap, mapInfo->proSubMapSize);
                return 0;
            }

            return -4;
        }

    };
}

#endif
