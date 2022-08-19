#ifndef __LYW_CODE_THREAD_SUB_TABLE_HPP__
#define __LYW_CODE_THREAD_SUB_TABLE_HPP__

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

        RWDelaySafeLock m_rwLock;

        MapInfo_t * m_mapInfo;
        //写发生在该map中 由Flush冲洗至m_mapInfo(仅仅交换地址即可）为了保证读的高效行 写数据生效延迟较大 通常为3s左右
        MapInfo_t * m_waitFlushMapInfo;
 
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
            MapInfo_t * mapInfo = NULL;

            int cliCount = expandMapInfo->cliCount;
            int msgCount = expandMapInfo->msgCount;
            int cliBitMapSize = cliCount / 8;
            int subInfoSize = expandMapInfo->subInfoSize;

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
            ::memeset(mapInfo, 0x00, totalSize);
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

            if (basicMapInfo->cliCount == cliCount)            
            {

            }


            return mapInfo;
        }

       

    public:

        ThreadSubTable()
        {
            pthread_mutex_init(&m_lock, NULL);

            MapInfo_t tmpMapInfo = {0};

            tmpMapInfo.cliCount = 16;

            tmpMapInfo.msgCount = 32;

            tmpMapInfo.cliBitMapSize = tmpMapInfo.cliCount / 8;

            tmpMapInfo.subInfoSize = 128;

            m_mapInfo = NULL;

            m_mapInfo = ExpandMap(tmpMapInfo, NULL);

            if (m_mapInfo != NULL)
            {
                m_waitFlushMapInfo = (MapInfo_t *)::malloc(m_mapInfo->totalSize)
                ::memcpy(m_waitFlushMapInfo, m_mapInfo, m_mapInfo->totalSize);
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

        int CliUnRegister()
        {
            ::pthread_mutex_lock(&m_lock);

            ::pthread_mutex_unlock(&m_lock);
            return 0;
        }

        int MsgRegister()
        {
            ::pthread_mutex_lock(&m_lock);

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
    };
}
#endif
