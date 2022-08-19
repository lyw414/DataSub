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
            //size为 MapInfo_t::msgCount
            int subInfoIndex[0];
        } CliSubMap_t;


        typedef struct _MsgSubMap {
            int st;
            int msgID;
            //size为 MapInfo_t::cliBitMapSize
            unsigned char cliBitMap[0];
        } MsgSubMap_t;


        typedef struct _MapInfo {
            //map size
            unsigned int totalSize;
            //初始定义为 16 按 16递增
            int cliCount;   
            //初始定义为 32 按 32递增
            int msgCount;
            //为 cliCount / 8
            int cliBitMapSize; 
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

        MapInfo_t * m_newMapInfo;
        std::map<int,int> m_newmsgMap;


        //写发生在该map中 由Flush冲洗至m_mapInfo(仅仅交换地址即可）为了保证读的高效行 写数据生效延迟较大 通常为3s左右
        MapInfo_t * m_waitFlushMapInfo;
        std::map<int,int> m_waitFlushMsgMap;
 
    private:
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

        MapInfo_t * ExpandMap(const MapInfo_t & expandMapInfo, const MapInfo_t * basicMapInfo)
        {
            unsigned int totalSize = 0;
            int size = 0;
            int size_ = 0;
            MapInfo_t * mapInfo = NULL;

            int cliCount = expandMapInfo.cliCount;
            int msgCount = expandMapInfo.msgCount;
            int cliBitMapSize = cliCount / 8;
            int subInfoSize = expandMapInfo.subInfoSize;


            int cpyCliCount = 0;
            int cpyMsgCount = 0;
            int cpyCliBitMapSize = 0;
            int cpySubInfoSize = 0;



            if (basicMapInfo != NULL 
               && basicMapInfo->cliCount == cliCount 
               && basicMapInfo->msgCount == msgCount 
               && basicMapInfo->subInfoSize == subInfoSize 
               )
            {
                return NULL;
            }

            totalSize = sizeof(MapInfo_t) + (sizeof(CliSubMap_t) + (sizeof(int) * msgCount)) * cliCount  + (sizeof(MsgSubMap_t) + cliBitMapSize) * msgCount + sizeof(SubInfo_t) * subInfoSize;

            mapInfo = (MapInfo_t *)::malloc(totalSize);

            //初始化
            ::memset(mapInfo, 0x00, totalSize);
            mapInfo->totalSize = totalSize;
            mapInfo->cliCount = cliCount;
            mapInfo->msgCount = msgCount;
            mapInfo->cliBitMapSize = cliBitMapSize;
            mapInfo->subInfoSize = subInfoSize;

            mapInfo->cliMap = (CliSubMap_t *)((unsigned char *)mapInfo + sizeof(MapInfo_t));
            size = sizeof(CliSubMap_t) + sizeof(int) * msgCount;

            for (int iLoop = 0; iLoop < cliCount; iLoop++) 
            {
                CliSubMap_t * cli = (CliSubMap_t *)((unsigned char *)mapInfo->cliMap + size * iLoop);
                
                for (int index = 0; index < msgCount; index++)
                {
                    cli->subInfoIndex[index] = -1;
                }
            }

            mapInfo->msgMap = (MsgSubMap_t *)((unsigned char *)mapInfo + sizeof(MapInfo_t) + (sizeof(CliSubMap_t) + (sizeof(int) * msgCount)) * cliCount);

            mapInfo->subInfo = (SubInfo_t *)((unsigned char *)mapInfo + sizeof(MapInfo_t) + (sizeof(CliSubMap_t) + (sizeof(int) * msgCount)) * cliCount  + (sizeof(MsgSubMap_t) + cliBitMapSize) * msgCount);


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

            if (basicMapInfo->cliBitMapSize < cliBitMapSize)
            {
                cpyCliBitMapSize = basicMapInfo->cliBitMapSize;
            }
            else
            {
                cpyCliBitMapSize = cliBitMapSize;
            }


            if (basicMapInfo->subInfoSize < subInfoSize)
            {
                cpySubInfoSize = basicMapInfo->subInfoSize;
            }
            else
            {
                cpySubInfoSize = subInfoSize;
            }

           
            size = sizeof(CliSubMap_t) + sizeof(int) * mapInfo->msgCount;
            size_ = sizeof(CliSubMap_t) + sizeof(int) * basicMapInfo->msgCount;

            for(int iLoop = 0; iLoop < cpyCliCount; iLoop++)
            {
                CliSubMap_t * cli = (CliSubMap_t *)((unsigned char *)(mapInfo->cliMap) + size * iLoop);
                CliSubMap_t * cli_ = (CliSubMap_t *)((unsigned char *)(basicMapInfo->cliMap) + size_ * iLoop);
                ::memcpy(cli, cli_, sizeof(CliSubMap_t));
                ::memcpy(cli->subInfoIndex, cli_->subInfoIndex, sizeof(int) * cpyMsgCount);

            }


            size = sizeof(MsgSubMap_t) + mapInfo->cliBitMapSize;
            size_ = sizeof(MsgSubMap_t) + basicMapInfo->cliBitMapSize;
            for(int iLoop = 0; iLoop < cpyMsgCount; iLoop++)
            {
                MsgSubMap_t * cli = (MsgSubMap_t *)((unsigned char *)(mapInfo->msgMap) + size * iLoop);
                MsgSubMap_t * cli_ = (MsgSubMap_t *)((unsigned char *)(mapInfo->msgMap) + size_ * iLoop);

                ::memcpy(cli, cli_, sizeof(MsgSubMap_t));

                ::memcpy(cli->cliBitMap, cli_->cliBitMap, cpyCliBitMapSize);
            }


            ::memcpy(mapInfo->subInfo, basicMapInfo->subInfo, sizeof(SubInfo_t) * cpySubInfoSize);


            return mapInfo;
        }
       

    public:

        ThreadSubTable()
        {
            pthread_mutex_init(&m_lock, NULL);

            pthread_mutex_init(&m_lock1, NULL);

            MapInfo_t tmpMapInfo = {0};

            tmpMapInfo.cliCount = 16;

            tmpMapInfo.msgCount = 32;

            tmpMapInfo.cliBitMapSize = tmpMapInfo.cliCount / 8;

            tmpMapInfo.subInfoSize = 128;

            m_mapInfo = NULL;

            m_mapInfo = ExpandMap(tmpMapInfo, NULL);

            if (m_mapInfo != NULL)
            {
                m_waitFlushMapInfo = (MapInfo_t *)::malloc(m_mapInfo->totalSize);
                ::memcpy(m_waitFlushMapInfo, m_mapInfo, m_mapInfo->totalSize);
                m_waitFlushMapInfo->cliMap = (CliSubMap_t *)((unsigned char *)m_waitFlushMapInfo + sizeof(MapInfo_t));
                m_waitFlushMapInfo->msgMap = (MsgSubMap_t *)((unsigned char *)m_waitFlushMapInfo + sizeof(MapInfo_t) + (sizeof(CliSubMap_t) + (sizeof(int) * m_waitFlushMapInfo->msgCount)) * m_waitFlushMapInfo->cliCount);
                m_waitFlushMapInfo->subInfo = (SubInfo_t *)((unsigned char *)m_waitFlushMapInfo + sizeof(MapInfo_t) + (sizeof(CliSubMap_t) + (sizeof(int) * m_waitFlushMapInfo->msgCount)) * m_waitFlushMapInfo->cliCount  + (sizeof(MsgSubMap_t) + m_waitFlushMapInfo->cliBitMapSize) * m_waitFlushMapInfo->msgCount);
            }
        }

        ~ThreadSubTable()
        {

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
            size = sizeof(CliSubMap_t) + sizeof(int) * m_waitFlushMapInfo->msgCount;
            for (int iLoop = 0; iLoop < m_waitFlushMapInfo->cliCount; iLoop++)
            {
                cli = (CliSubMap_t *)((unsigned char *)(m_waitFlushMapInfo->cliMap) + size * iLoop);
                if (cli->st == 0)
                {
                    //找到空闲
                    cli->st = 1; 
                    ::pthread_mutex_unlock(&m_lock);
                    return iLoop;
                }
            }

            //cli分配完成 需要扩展
            cliIndex = m_waitFlushMapInfo->cliCount;

            tmpMapInfo.cliCount = m_waitFlushMapInfo->cliCount;

            tmpMapInfo.msgCount = m_waitFlushMapInfo->msgCount;

            tmpMapInfo.cliBitMapSize = m_waitFlushMapInfo->cliCount / 8;
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

                cli = (CliSubMap_t *)((unsigned char *)(m_waitFlushMapInfo->cliMap) + size * cliIndex);
                cli->st = 1;
            }

            ::pthread_mutex_unlock(&m_lock);
            return cliIndex;
        }

        int CliUnRegister(int cliIndex)
        {
            CliSubMap_t * cli = NULL;

            int size = 0;

            int count = 0;

            if (m_waitFlushMapInfo == NULL)
            {
                return -1;
            }

            ::pthread_mutex_lock(&m_lock);

            size = sizeof(CliSubMap_t) + sizeof(int) * m_waitFlushMapInfo->msgCount;

            if (cliIndex < m_waitFlushMapInfo->cliCount)
            {
                cli = (CliSubMap_t *)((unsigned char *)(m_waitFlushMapInfo->cliMap) + cliIndex * size);

                cli->st = 0;
                
                //检测是否需要回收资源
                for (int iLoop = 16; iLoop <  m_waitFlushMapInfo->cliCount; iLoop++)
                {
                    cli = (CliSubMap_t *)((unsigned char *)(m_waitFlushMapInfo->cliMap) + iLoop * size);
                    if(cli->st == 0)
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
                    tmp.cliBitMapSize = tmp.cliCount / 8;
                    tmp.subInfoSize = m_waitFlushMapInfo->subInfoSize;

                    mapInfo = ExpandMap(tmp, m_waitFlushMapInfo);
                    
                    ::free(m_waitFlushMapInfo);

                    m_waitFlushMapInfo = mapInfo;
                }
            }

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

            SubInfo_t * subInfo = NULL;
            int subIndex = 0;

            if (m_waitFlushMapInfo == NULL)
            {
                return -1;
            }

            ::pthread_mutex_lock(&m_lock);

            if (cliIndex > m_waitFlushMapInfo->cliCount)
            {
                return -1;
            }

            cliSize = sizeof(CliSubMap_t) + sizeof(int) * m_waitFlushMapInfo->msgCount;

            cli = (CliSubMap_t *)((unsigned char *)(m_waitFlushMapInfo->cliMap) + cliIndex * cliSize);

            if (m_waitFlushMsgMap.find(msgID) ==  m_waitFlushMsgMap.end())
            {
                //消息未登记
                msgSize = sizeof(MsgSubMap_t) +  m_waitFlushMsgMap->cliBitMapSize;
                for (int iLoop = 0; iLoop < m_waitFlushMapInfo->msgCount; iLoop++)
                {
                    msg = (MsgSubMap_t *)((unsigned char *)(m_waitFlushMapInfo->msgMap) + iLoop * msgSize);
                    if (msg->st == 0)
                    {
                        msgIndex = iLoop;
                        m_waitFlushMsgMap[msgID] = iLoop;
                        msg->st = 1;
                        msg->msgID = msgID;
                        SetBit(msg->cliBitMap, cliIndex, 1);
                        break;
                    }
                }

                if (msgIndex == -1)
                {
                    //

                }
            }
            else
            {
                //消息已经登记 
                msgIndex = m_waitFlushMsgMap[msgID];

            }

            ::pthread_mutex_unlock(&m_lock);
            return 0;
        }

        int MsgUnRegister()
        {
            ::pthread_mutex_lock(&m_lock);

            ::pthread_mutex_unlock(&m_lock);
            return 0;
        }

        void Flush()
        {
            ::pthread_mutex_lock(&m_lock1);

            ::pthread_mutex_lock(&m_lock);
            m_newMapInfo = m_waitFlushMapInfo;
            m_newMsgMap = m_waitFlushMsgMap;

            m_waitFlushMapInfo = (MapInfo_t *)::malloc(m_newMapInfo->totalSize);
            ::memcpy(m_waitFlushMapInfo, m_mapInfo, m_newMapInfo->totalSize);

            m_waitFlushMapInfo->cliMap = (CliSubMap_t *)((unsigned char *)m_waitFlushMapInfo + sizeof(MapInfo_t));
            m_waitFlushMapInfo->msgMap = (MsgSubMap_t *)((unsigned char *)m_waitFlushMapInfo + sizeof(MapInfo_t) + (sizeof(CliSubMap_t) + (sizeof(int) * m_waitFlushMapInfo->msgCount)) * m_waitFlushMapInfo->cliCount);
            m_waitFlushMapInfo->subInfo = (SubInfo_t *)((unsigned char *)m_waitFlushMapInfo + sizeof(MapInfo_t) + (sizeof(CliSubMap_t) + (sizeof(int) * m_waitFlushMapInfo->msgCount)) * m_waitFlushMapInfo->cliCount  + (sizeof(MsgSubMap_t) + m_waitFlushMapInfo->cliBitMapSize) * m_waitFlushMapInfo->msgCount);

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

        int QuerySubInfo(int msgIndex, int cliIndex, SubInfo_t *  SubInfo)
        {
            return 0;
        }

        int QueryCliSubMap(int msgIndex, void * bitMap, unsigned int sizeOfMap)
        {
            return 0;
        }

        bool CheckBitMap(void * bitMap, unsigned int lenOfMap, int checkIndex)
        {
            return false;
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
            printf("bitMapSize  [%d]\n", m_mapInfo->cliBitMapSize);
            printf("subInfoSize [%d]\n", m_mapInfo->subInfoSize);
            printf("mapInfo     [%p]\n", m_mapInfo);
            printf("cliMap      [%p]\n", m_mapInfo->cliMap);
            printf("msgMap      [%p]\n", m_mapInfo->msgMap);
            printf("subInfo     [%p]\n", m_mapInfo->subInfo);


            printf("\n*************CLI_Map******************\n");
             
            size = sizeof(CliSubMap_t) + sizeof(int) * m_mapInfo->msgCount;
            for (int iLoop = 0; iLoop < m_mapInfo->cliCount; iLoop++)
            {
                CliSubMap_t * cli = (CliSubMap_t *)((unsigned char *)m_mapInfo->cliMap + size * iLoop);
                printf("%03d ST %d IndexMap[", iLoop, cli->st);
                for (int index = 0; index < m_mapInfo->msgCount; index++)
                {
                    printf("%03d:%d ", index, cli->subInfoIndex[index]);
                }
                printf("]\n");
            }

            printf("\n*************MSG_Map******************\n");

            size = sizeof(MsgSubMap_t) + m_mapInfo->cliBitMapSize;
            for (int iLoop = 0; iLoop < m_mapInfo->msgCount; iLoop++)
            {
                MsgSubMap_t * cli = (MsgSubMap_t *)((unsigned char *)m_mapInfo->msgMap + size * iLoop);
                printf("%03d ST %d CliBitMap[", iLoop, cli->st);
                for (int index = 0; index < m_mapInfo->cliBitMapSize; index++)
                {
                    printf("%02X ",cli->cliBitMap[index]);
                }
                printf("]\n");
            }

            printf("\n*************SUB_Map******************\n");
            for (int iLoop = 0; iLoop < m_mapInfo->subInfoSize; iLoop++)
            {
                SubInfo_t * cli = m_mapInfo->subInfo;
                bool b1;
                bool b2;
                
                b1 = (cli[iLoop].handleCB != NULL);

                b2 = (cli[iLoop].errorCB != NULL);

                printf("%03d ST %d UserParam %p HandleCB[%d] ErrCB[%d]\n",iLoop, cli[iLoop].st, cli[iLoop].userParam, b1, b2);

            }
        }
    };
}
#endif
