#ifndef __LYW_CODE_THREAD_SUB_TABLE_HPP__
#define __LYW_CODE_THREAD_SUB_TABLE_HPP__

#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>


#include "DataSubFunc.hpp"
#include "RWLock.hpp"


namespace LYW_CODE
{
    class ThreadSubTable
    {
    public:
#pragma pack(1)
        typedef struct _SubInfo {
            int st;
            void * userParam;
            Function3<void(void *, unsigned int, void *)> handleCB;
            Function3<void(void *, unsigned int, void *)> errorCB;
        } SubInfo_t;
#pragma pack()
    private:

#pragma pack(1)
        typedef struct _CliSubMap {
            int st;
        } CliSubMap_t;

        
        typedef struct _MsgSubMap {
            int st;
            int msgID;
            SubInfo_t * cliSubInfo[0];
        } MsgSubMap_t;


        typedef struct _MapInfo {
            //map size
            unsigned int totalSize;
            //初始定义为 16 按 16递增
            int cliCount;   
            //初始定义为 32 按 32递增
            int msgCount;
            //初始定义为 128 按 128递增
            int subInfoSize;

            //客户注册map开始地址
            CliSubMap_t * cliMap;
            //消息注册map开始地址
            MsgSubMap_t * msgMap;
            //订阅详情（数组）开始地址 下标为索引 在每个cli后存在一个msgMap内登记了该索引
            SubInfo_t * subInfo;



        } MapInfo_t;
#pragma pack()
    private:
        pthread_mutex_t m_lock;

        pthread_mutex_t m_lock1;

        RWDelaySafeLock m_rwLock;

        MapInfo_t * m_mapInfo;
        std::map<int,int> m_msgMap;
        std::map<int,int> m_proMsgMap;

        MapInfo_t * m_newMapInfo;
        std::map<int,int> m_newMsgMap;
        std::map<int,int> m_newProMsgMap;


        //写发生在该map中 由Flush冲洗至m_mapInfo(仅仅交换地址即可）为了保证读的高效行 写数据生效延迟较大 通常为3s左右
        MapInfo_t * m_waitFlushMapInfo;
        std::map<int,int> m_waitFlushMsgMap;
        std::map<int,int> m_waitFlushProMsgMap;
        
        int m_isNeedFlush;

        int m_isProMsgNeedFlush;

        int m_tag;

        pthread_t m_thread;

 
    private:
        static void * DoFlush_(void * ptr)
        {
            ThreadSubTable * self = (ThreadSubTable *)ptr;
            if (self != NULL)
            {
                self->DoFlush();
            }
            return NULL;
        }

        void DoFlush()
        {
            while (m_tag == 1)     
            {

                if (m_isNeedFlush != 0)
                {
                    Flush();
                }
                else if (m_isProMsgNeedFlush != 0)
                {
                    FlushProMsgMap();
                }
                else
                {
                    ::sleep(1);
                }
            }
        }

        MapInfo_t * ExpandMap(const MapInfo_t & expandMapInfo, const MapInfo_t * basicMapInfo)
        {
            unsigned int totalSize = 0;
            int size = 0;
            int size_ = 0;
            MapInfo_t * mapInfo = NULL;

            int cliCount = expandMapInfo.cliCount;
            int msgCount = expandMapInfo.msgCount;
            int subInfoSize = expandMapInfo.subInfoSize;


            int cpyCliCount = 0;
            int cpyMsgCount = 0;
            int cpySubInfoSize = 0;



            if (basicMapInfo != NULL 
               && basicMapInfo->cliCount == cliCount 
               && basicMapInfo->msgCount == msgCount 
               && basicMapInfo->subInfoSize == subInfoSize 
               )
            {
                return NULL;
            }

            totalSize = sizeof(MapInfo_t) + sizeof(CliSubMap_t) * cliCount  + (sizeof(MsgSubMap_t) + sizeof(SubInfo_t *) * cliCount) * msgCount + sizeof(SubInfo_t) * subInfoSize;

            mapInfo = (MapInfo_t *)::malloc(totalSize);

            //初始化
            ::memset(mapInfo, 0x00, totalSize);
            mapInfo->totalSize = totalSize;
            mapInfo->cliCount = cliCount;
            mapInfo->msgCount = msgCount;
            mapInfo->subInfoSize = subInfoSize;

            mapInfo->cliMap = (CliSubMap_t *)((unsigned char *)mapInfo + sizeof(MapInfo_t));

            mapInfo->msgMap = (MsgSubMap_t *)((unsigned char *)mapInfo + sizeof(MapInfo_t) + sizeof(CliSubMap_t) * cliCount);

            mapInfo->subInfo = (SubInfo_t *)((unsigned char *)mapInfo + sizeof(MapInfo_t) + sizeof(CliSubMap_t) * cliCount + (sizeof(MsgSubMap_t) + sizeof(SubInfo_t *) * cliCount) * msgCount);

            //拷贝旧值
            if (basicMapInfo == NULL)
            {
                return mapInfo;
            }

            
            //取小
            if (basicMapInfo->cliCount < cliCount)
            {
               cpyCliCount = basicMapInfo->cliCount;
            }
            else
            {
                cpyCliCount = cliCount;
            }

            if (basicMapInfo->msgCount < msgCount)
            {
                cpyMsgCount = basicMapInfo->msgCount;
            }
            else
            {
                cpyMsgCount = msgCount;
            }

            if (basicMapInfo->subInfoSize < subInfoSize)
            {
                cpySubInfoSize = basicMapInfo->subInfoSize;
            }
            else
            {
                cpySubInfoSize = subInfoSize;
            }

           
            
            //cliMap copy
            ::memcpy(mapInfo->cliMap, basicMapInfo->cliMap, sizeof(CliSubMap_t) * cpyCliCount);
            
            
            //msgMap copy
            size = sizeof(MsgSubMap_t) + mapInfo->cliCount * sizeof(SubInfo_t *);
            size_ = sizeof(MsgSubMap_t) + basicMapInfo->cliCount * sizeof(SubInfo_t *);

            for(int iLoop = 0; iLoop < cpyMsgCount; iLoop++)
            {
                MsgSubMap_t * cli = (MsgSubMap_t *)((unsigned char *)(mapInfo->msgMap) + size * iLoop);
                MsgSubMap_t * cli_ = (MsgSubMap_t *)((unsigned char *)(mapInfo->msgMap) + size_ * iLoop);

                ::memcpy(cli, cli_, sizeof(MsgSubMap_t));

                ::memcpy(cli->cliSubInfo, cli_->cliSubInfo, cpyCliCount * sizeof(SubInfo_t *));
            }


            ::memcpy(mapInfo->subInfo, basicMapInfo->subInfo, sizeof(SubInfo_t) * cpySubInfoSize);

            return mapInfo;
        }
       

    public:

        ThreadSubTable()
        {
            m_tag = 1;

            m_isNeedFlush = 0;
            
            m_isProMsgNeedFlush = 0;

            pthread_mutex_init(&m_lock, NULL);

            pthread_mutex_init(&m_lock1, NULL);

            MapInfo_t tmpMapInfo = {0};

            tmpMapInfo.cliCount = 16;

            tmpMapInfo.msgCount = 32;

            tmpMapInfo.subInfoSize = 128;

            m_mapInfo = NULL;

            m_mapInfo = ExpandMap(tmpMapInfo, NULL);

            if (m_mapInfo != NULL)
            {
                m_waitFlushMapInfo = (MapInfo_t *)::malloc(m_mapInfo->totalSize);
                ::memcpy(m_waitFlushMapInfo, m_mapInfo, m_mapInfo->totalSize);
                m_waitFlushMapInfo->cliMap = (CliSubMap_t *)((unsigned char *)m_waitFlushMapInfo + sizeof(MapInfo_t));

                m_waitFlushMapInfo->msgMap = (MsgSubMap_t *)((unsigned char *)m_waitFlushMapInfo + sizeof(MapInfo_t) + sizeof(CliSubMap_t) * m_waitFlushMapInfo->cliCount);

                m_waitFlushMapInfo->subInfo = (SubInfo_t *)((unsigned char *)m_waitFlushMapInfo + sizeof(MapInfo_t) + sizeof(CliSubMap_t) * m_waitFlushMapInfo->cliCount  + (sizeof(MsgSubMap_t) + sizeof(SubInfo_t *) * m_waitFlushMapInfo->cliCount) * m_waitFlushMapInfo->msgCount);
            }

            ::pthread_create(&m_thread, NULL, ThreadSubTable::DoFlush_, this);

        }

        ~ThreadSubTable()
        {
            if (m_tag == 1)
            {
                m_tag = 0;
                void * res;
                pthread_join(m_thread , &res);
            }
        }

        int SetProMsgMap(int msgID, int msgIndex)
        {
            ::pthread_mutex_lock(&m_lock);
            m_waitFlushProMsgMap[msgID] = msgIndex;
            m_isProMsgNeedFlush = 1;
            ::pthread_mutex_unlock(&m_lock);
            return 0;
        }

        int GetProMsgMap(int msgID)
        {
            int st = 0;
            int msgIndex = -1;
            std::map<int, int> ::iterator it;

            st = m_rwLock.ReadLock();
            if (st == 0)
            {
                if ((it = m_proMsgMap.find(msgID)) != m_proMsgMap.end())
                {
                    msgIndex = it->second;
                }

            }
            else
            {
                if ((it = m_newProMsgMap.find(msgID)) != m_newProMsgMap.end())
                {
                    msgIndex = it->second;
                }

            }
            m_rwLock.ReadUnLock(st);

            return msgIndex;
        }

        void FlushProMsgMap()
        {
            ::pthread_mutex_lock(&m_lock1);
            ::pthread_mutex_lock(&m_lock);
            m_isProMsgNeedFlush = 0;
            m_newProMsgMap = m_waitFlushProMsgMap;
            ::pthread_mutex_unlock(&m_lock);

            MapInfo_t *tmp = m_mapInfo;
            m_rwLock.WriteLock(500000);
            m_proMsgMap = m_newProMsgMap;
            m_rwLock.WriteUnLock(500000);
            ::pthread_mutex_unlock(&m_lock1);
        }

        int CliRegister()
        {

            CliSubMap_t * cli = NULL;

            MapInfo_t * mapInfo = NULL;

            MapInfo_t tmpMapInfo = {0};

            int cliIndex = 0;

            int size = 0;

            if (m_mapInfo == NULL)
            {
                return -1;
            }

            ::pthread_mutex_lock(&m_lock);

            cli = m_waitFlushMapInfo->cliMap;
            for (int iLoop = 0; iLoop < m_waitFlushMapInfo->cliCount; iLoop++)
            {
                if (cli[iLoop].st == 0)
                {
                    //找到空闲
                    cli[iLoop].st = 1; 
                    ::pthread_mutex_unlock(&m_lock);
                    return iLoop;
                }
            }

            //cli分配完成 需要扩展
            cliIndex = m_waitFlushMapInfo->cliCount;

            tmpMapInfo.cliCount = m_waitFlushMapInfo->cliCount + 16;
            tmpMapInfo.msgCount = m_waitFlushMapInfo->msgCount;

            tmpMapInfo.subInfoSize = m_waitFlushMapInfo->subInfoSize;
            mapInfo = ExpandMap(tmpMapInfo, m_waitFlushMapInfo);
            
            if (mapInfo == NULL)
            {
                cliIndex = -1;
            }
            else
            {
                //cliIndex 可用
                ::free(m_waitFlushMapInfo);
                m_waitFlushMapInfo = mapInfo;
                m_waitFlushMapInfo->cliMap[cliIndex].st = 1;
            }

            m_isNeedFlush = 1;
            ::pthread_mutex_unlock(&m_lock);
            return cliIndex;
        }

        int CliUnRegister(int cliIndex)
        {
            CliSubMap_t * cli = NULL;

            int count = 0;

            if (m_waitFlushMapInfo == NULL)
            {
                return -1;
            }

            ::pthread_mutex_lock(&m_lock);

            cli = m_waitFlushMapInfo->cliMap;

            if (cliIndex < m_waitFlushMapInfo->cliCount)
            {
                cli[cliIndex].st = 0;
                
                //检测是否需要回收资源
                for (int iLoop = 16; iLoop <  m_waitFlushMapInfo->cliCount; iLoop++)
                {
                    if(cli[iLoop].st == 0)
                    {
                        count++;
                    }
                    else
                    {
                        count = 0;
                    }
                }

                if (count / 16 > 0)
                {
                    MapInfo_t tmp = {0};

                    MapInfo_t * mapInfo = NULL;

                    tmp.cliCount = m_waitFlushMapInfo->cliCount - (count / 16) * 16;
                    tmp.msgCount = m_waitFlushMapInfo->msgCount;
                    tmp.subInfoSize = m_waitFlushMapInfo->subInfoSize;
                    mapInfo = ExpandMap(tmp, m_waitFlushMapInfo);
                    
                    ::free(m_waitFlushMapInfo);

                    m_waitFlushMapInfo = mapInfo;
                }
            }
            m_isNeedFlush = 1;

            ::pthread_mutex_unlock(&m_lock);

            return 0;
        }

        int MsgRegister(int cliIndex, int msgID, Function3<void(void *, unsigned int, void *)> handleCB, Function3<void(void *, unsigned int, void *)> ErrorCB, void * userParam)
        {
            CliSubMap_t * cli = NULL;
            int cliSize = 0;

            MsgSubMap_t * msg = NULL;

            int msgSize = 0;
            int msgIndex = -1;

            MapInfo_t tmp = {0};

            MapInfo_t * mapInfo = NULL;


            SubInfo_t * subInfo = NULL;
            int subInfoIndex = -1;

            if (m_waitFlushMapInfo == NULL)
            {
                return -1;
            }

            ::pthread_mutex_lock(&m_lock);

            if (cliIndex > m_waitFlushMapInfo->cliCount || m_waitFlushMapInfo->cliMap[cliIndex].st == 0)
            {
                return -1;
            }

            cli = m_waitFlushMapInfo->cliMap + cliIndex;

            msgSize = sizeof(MsgSubMap_t) +  m_waitFlushMapInfo->cliCount * sizeof(SubInfo_t *);
            if (m_waitFlushMsgMap.find(msgID) ==  m_waitFlushMsgMap.end())
            {
                //消息未登记

                for (int iLoop = 0; iLoop < m_waitFlushMapInfo->msgCount; iLoop++)
                {
                    msg = (MsgSubMap_t *)((unsigned char *)(m_waitFlushMapInfo->msgMap) + iLoop * msgSize);
                    if (msg->st == 0)
                    {
                        msgIndex = iLoop;
                        //msg->cliSubInfo[cliIndex] = NULL;
                        break;
                    }
                }
            }
            else
            {
                //消息已经登记 
                msgIndex = m_waitFlushMsgMap[msgID];
                msg = (MsgSubMap_t *)((unsigned char *)(m_waitFlushMapInfo->msgMap) + msgIndex * msgSize);
                subInfo = msg->cliSubInfo[cliIndex];
            }


            if (subInfo == NULL)
            {
                for (int iLoop = 0; iLoop < m_waitFlushMapInfo->subInfoSize; iLoop++)
                {
                    if (m_waitFlushMapInfo->subInfo[iLoop].st == 0)
                    {
                        subInfo = &(m_waitFlushMapInfo->subInfo[iLoop]);
                        break;
                    }
                }
            }

            if (msgIndex == -1 || subInfo == NULL)
            {
                //扩展mapInfo
                tmp.cliCount = m_waitFlushMapInfo->cliCount;

                if (msgIndex == -1)
                {
                    msgIndex = m_waitFlushMapInfo->msgCount;
                    tmp.msgCount = m_waitFlushMapInfo->msgCount + 32;
                }
                else
                {
                    tmp.msgCount = m_waitFlushMapInfo->msgCount;
                }

                if (subInfo == NULL)
                {
                    subInfoIndex = m_waitFlushMapInfo->subInfoSize;
                    tmp.subInfoSize = m_waitFlushMapInfo->subInfoSize + 128;
                }
                else
                {
                    tmp.subInfoSize = m_waitFlushMapInfo->subInfoSize;
                }


                mapInfo = ExpandMap(tmp, m_waitFlushMapInfo);

                ::free(m_waitFlushMapInfo);

                m_waitFlushMapInfo = mapInfo;

                if (subInfo == NULL)
                {
                    subInfo = &(m_waitFlushMapInfo->subInfo[subInfoIndex]);
                }
            }

            msg = (MsgSubMap_t *)((unsigned char *)(m_waitFlushMapInfo->msgMap) + msgIndex * msgSize);
            m_waitFlushMsgMap[msgID] = msgIndex;
            msg->st = 1;
            msg->msgID = msgID;

            msg->cliSubInfo[cliIndex] = subInfo;

            msg->cliSubInfo[cliIndex]->st = 1;
            msg->cliSubInfo[cliIndex]->userParam = userParam;
            msg->cliSubInfo[cliIndex]->handleCB = handleCB;
            msg->cliSubInfo[cliIndex]->errorCB = ErrorCB;
            m_isNeedFlush = 1;
            ::pthread_mutex_unlock(&m_lock);
            return 0;
        }

        int MsgUnRegister(int cliIndex, int msgID)
        {

            MsgSubMap_t * msg = NULL;

            int msgIndex = 0;

            if (m_waitFlushMapInfo == NULL)
            {
                return -1;
            }

            ::pthread_mutex_lock(&m_lock);
            m_isNeedFlush = 1;

            if (cliIndex >= m_waitFlushMapInfo->cliCount || msgIndex >= m_waitFlushMapInfo->msgCount || m_waitFlushMsgMap.find(msgID) ==  m_waitFlushMsgMap.end())
            {
                ::pthread_mutex_unlock(&m_lock);
                return -1;
            }

            msgIndex = m_waitFlushMsgMap[msgID];

            msg = (MsgSubMap_t *)((unsigned char *)(m_waitFlushMapInfo->msgMap) +  (sizeof(MsgSubMap_t) + sizeof(SubInfo_t *) * m_waitFlushMapInfo->cliCount) * msgIndex);

            if (msg->cliSubInfo[cliIndex] != NULL)
            {
                msg->cliSubInfo[cliIndex]->st = 0;
                msg->cliSubInfo[cliIndex] = NULL;
            }

            for (int iLoop = 0; iLoop < m_waitFlushMapInfo->cliCount; iLoop++)
            {
                if (msg->cliSubInfo[iLoop] != NULL)
                {
                    ::pthread_mutex_unlock(&m_lock);
                    return 0;
                }

            }
            
            //没有客户端订阅该消息 清理掉
            m_waitFlushMsgMap.erase(msg->msgID);
            msg->st = 0;

            ::pthread_mutex_unlock(&m_lock);
            return 0;
        }

        void Flush()
        {
            ::pthread_mutex_lock(&m_lock1);

            ::pthread_mutex_lock(&m_lock);
            m_isNeedFlush = 0;
            m_newMapInfo = m_waitFlushMapInfo;
            m_newMsgMap = m_waitFlushMsgMap;

            m_waitFlushMapInfo = (MapInfo_t *)::malloc(m_newMapInfo->totalSize);
            ::memcpy(m_waitFlushMapInfo, m_mapInfo, m_newMapInfo->totalSize);

            m_waitFlushMapInfo->cliMap = (CliSubMap_t *)((unsigned char *)m_waitFlushMapInfo + sizeof(MapInfo_t));

            m_waitFlushMapInfo->msgMap = (MsgSubMap_t *)((unsigned char *)m_waitFlushMapInfo + sizeof(MapInfo_t) + sizeof(CliSubMap_t) * m_waitFlushMapInfo->cliCount);

            m_waitFlushMapInfo->subInfo = (SubInfo_t *)((unsigned char *)m_waitFlushMapInfo + sizeof(MapInfo_t) + sizeof(CliSubMap_t) * m_waitFlushMapInfo->cliCount  + (sizeof(MsgSubMap_t) + m_waitFlushMapInfo->cliCount * sizeof(MsgSubMap_t)) * m_waitFlushMapInfo->msgCount);

            ::pthread_mutex_unlock(&m_lock);

            MapInfo_t *tmp = m_mapInfo;
            m_rwLock.WriteLock(1500000);
            m_mapInfo = m_newMapInfo;
            m_msgMap = m_newMsgMap;
            m_rwLock.WriteUnLock(1500000);
            
            if (tmp != NULL)
            {
                ::free(tmp);
            }

            ::pthread_mutex_unlock(&m_lock1);
        }


        int QuerySubInfo(int cliIndex, int msgIndex, SubInfo_t * subInfo)
        {
            int st = 0;
            
            MapInfo_t * mapInfo = NULL;

            MsgSubMap_t * msg = NULL;

            st = m_rwLock.ReadLock();

            if (st == 0)
            {
                if (m_mapInfo == NULL)
                {
                    m_rwLock.ReadUnLock(st);
                    return -1;
                }

                mapInfo = m_mapInfo;
            }
            else
            {
                if (m_newMapInfo == NULL)
                {
                    m_rwLock.ReadUnLock(st);
                    return -1;
                }

                mapInfo = m_newMapInfo;
            }
            
            if (cliIndex >= mapInfo->cliCount || msgIndex >= mapInfo->msgCount)
            {
                m_rwLock.ReadUnLock(st);
                return -1;
            }

            msg = (MsgSubMap_t *)((unsigned char *)mapInfo->msgMap + (sizeof(MsgSubMap_t) + sizeof(SubInfo_t *) * mapInfo->cliCount) * msgIndex);
                        
            if (mapInfo->cliMap[cliIndex].st == 0 || msg->st == 0 || msg->cliSubInfo[cliIndex] == NULL)
            {
                m_rwLock.ReadUnLock(st);
                return -2;
            }
            
            if (subInfo != NULL)
            {
                subInfo->st = msg->cliSubInfo[cliIndex]->st ;
                subInfo->userParam = msg->cliSubInfo[cliIndex]->userParam;
                subInfo->handleCB= msg->cliSubInfo[cliIndex]->handleCB;
                subInfo->errorCB= msg->cliSubInfo[cliIndex]->errorCB;
            }
            m_rwLock.ReadUnLock(st);

            return msgIndex;
        }



        int QueryMsgIsSub(int cliIndex, int msgID)
        {
            int st = 0;
            
            MapInfo_t * mapInfo = NULL;

            MsgSubMap_t * msg = NULL;

            int msgIndex = 0;

            st = m_rwLock.ReadLock();

            if (st == 0)
            {
                if (m_mapInfo == NULL)
                {
                    m_rwLock.ReadUnLock(st);
                    return -1;
                }

 
                //无写等待 使用 m_mapInfo
                mapInfo = m_mapInfo;
                if (m_msgMap.find(msgID) == m_msgMap.end())
                {
                    m_rwLock.ReadUnLock(st);
                    return -1;
                }

                msgIndex = m_msgMap[msgID];
            }
            else
            {
                if (m_newMapInfo == NULL)
                {
                    m_rwLock.ReadUnLock(st);
                    return -1;
                }

                //有写等待 使用 m_newMapInfo
                mapInfo = m_newMapInfo;
                if (m_newMsgMap.find(msgID) == m_newMsgMap.end())
                {
                    m_rwLock.ReadUnLock(st);
                    return -1;
                }
                msgIndex = m_newMsgMap[msgID];
            }
            
            if (cliIndex >= mapInfo->cliCount || msgIndex >= mapInfo->msgCount)
            {
                m_rwLock.ReadUnLock(st);
                return -1;
            }

            msg = (MsgSubMap_t *)((unsigned char *)mapInfo->msgMap + (sizeof(MsgSubMap_t) + sizeof(SubInfo_t *) * mapInfo->cliCount) * msgIndex);
                        
            if (mapInfo->cliMap[cliIndex].st == 0 || msg->st == 0 || msg->cliSubInfo[cliIndex] == NULL)
            {
                m_rwLock.ReadUnLock(st);
                return -2;
            }
            
            m_rwLock.ReadUnLock(st);

            return msgIndex;
        }

        int CliCount()
        {
            int st = 0;
            int cliCount = 0;
            if (m_mapInfo == NULL)
            {
                return -1;
            }

            st = m_rwLock.ReadLock();

            if (st == 0)
            {
                //无写等待 使用 m_mapInfo
                cliCount = m_mapInfo->cliCount;
            }
            else
            {
                //有写等待 使用 m_newMapInfo
                cliCount = m_newMapInfo->cliCount;
            }
            m_rwLock.ReadUnLock(st);

            return cliCount;
        }


        void ShowInfo()
        {
            int size = 0;
            if (m_mapInfo == NULL)
            {
                return;
            }

            printf("Map Info::\n");
            printf("totalSize   [%d]\n", m_mapInfo->totalSize);
            printf("cliCount    [%d]\n", m_mapInfo->cliCount);
            printf("msgCount    [%d]\n", m_mapInfo->msgCount);
            printf("subInfoSize [%d]\n", m_mapInfo->subInfoSize);
            printf("mapInfo     [%p]\n", m_mapInfo);
            printf("cliMap      [%p]\n", m_mapInfo->cliMap);
            printf("msgMap      [%p]\n", m_mapInfo->msgMap);
            printf("subInfo     [%p]\n", m_mapInfo->subInfo);


            printf("\n*************CLI_Map******************\n");
             
            size = sizeof(CliSubMap_t) + sizeof(int) * m_mapInfo->msgCount;
            for (int iLoop = 0; iLoop < m_mapInfo->cliCount; iLoop++)
            {
                CliSubMap_t * cli = m_mapInfo->cliMap + iLoop;
                printf("%03d ST %d\n", iLoop, cli->st);
            }

            printf("\n*************MSG_Map******************\n");

            size = sizeof(MsgSubMap_t) + m_mapInfo->cliCount * sizeof(SubInfo_t *);
            for (int iLoop = 0; iLoop < m_mapInfo->msgCount; iLoop++)
            {
                MsgSubMap_t * cli = (MsgSubMap_t *)((unsigned char *)m_mapInfo->msgMap + size * iLoop);
                printf("%03d ST %d MsgID %d CliSubInfo::\n", iLoop, cli->st, cli->msgID);
                for (int index = 0; index < m_mapInfo->cliCount; index++)
                {
                    printf("        cliIndx %03d  ADDR[%p] ", index, cli->cliSubInfo[index]);
                    if (cli->cliSubInfo[index] != NULL)
                    {
                        bool b1;
                        bool b2;
                        
                        b1 = (cli->cliSubInfo[index]->handleCB != NULL);
                        b2 = (cli->cliSubInfo[index]->errorCB != NULL);
                        printf("ST %d UserParam %p HandleCB[%d] ErrCB[%d]\n", cli->cliSubInfo[index]->st, cli->cliSubInfo[index]->userParam,  b1, b2);
                    }
                    else
                    {
                        printf("\n");
                    }
                }
            }
        }
    };
}
#endif
