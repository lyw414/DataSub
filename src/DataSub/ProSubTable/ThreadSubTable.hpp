#ifndef __LYW_CODE_THREAD_SUB_TABLE_HPP_
#define __LYW_CODE_THREAD_SUB_TABLE_HPP_

#include "DataSubFunc.hpp"
#include <map>

namespace LYW_CODE
{
    //数据结构为 (ClientSubMap_t + ClientSubInfoIndex_t)[] + MsgSubMap_t[]
    class ThreadSubTable
    {
    public:
#pragma pack(1)
        typedef _ClientSubInfo {
            int st;
            void * userParam;
            Function3<void(void *, unsigned int, void *)> handleCB;
            Function3<void(void *, unsigned int, void *)> ErrorCB;
        } ClientSubInfo_t;
#pragma pack()

    private:

#pragma pack(1)
        typedef struct _ClientSubMap {
            int st;
        } ClientSubMap_t;

        typedef struct _ClientSubInfoIndex{
            int index;
        } ClientSubInfoIndex_t;

        typedef struct _MsgSubInfo {
            int msgID;
            int st;
        } MsgSubMap_t;


        typedef struct _MapInfo {
            int cliCount;
            int msgCount;
            int subMapSize;
            unsigned int totalSize;
        } MapInfo_t;


#pragma pack()
    private:
        //写阻塞锁 保证读效率
        pthread_mutex_t m_lock;
        pthread_mutex_t m_readLock;

        unsigned int m_readRecord;
        unsigned int m_IsWrite;


        MapInfo_t * m_map;

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

        MapInfo_t * ExpandMap(const MapInfo_t & mapInfo)
        {
            MapInfo_t * tmp = NULL;
            int totalSize = 0;
            unsigned int readRecord = 0;

            ::pthread_mutex_lock(&m_lock);

            if (m_map == NULL || mapInfo.cliCount != m_map.cliCount || mapInfo.msgCount != m_map.msgCount) 
            {
                //创建新的map 
                totalSize = sizeof(MapInfo_t) + mapInfo.cliCount * (sizeof(ClientSubMap_t) + sizeof(ClientSubInfoIndex_t)) + mapInfo.msgCount * (sizeof(MapInfo_t) + mapInfo.cliCount/8);
                tmp = (MapInfo_t *)::malloc(totalSize);
                ::memset(tmp, 0x00, totalSize);
                tmp->cliCount = mapInfo.cliCount;
                tmp->msgCount = mapInfo.msgCount;
                tmp->subMapSize = tmp->cliCount / 8;
                tmp->totalSize = totalSize;
                //拷贝旧数据
                if (m_map != NULL)
                {
                    
                }

                return tmp;
            }
            else
            {
               //无需扩容
               return NULL;
            }
        }

    public:
        ThreadSubTable()
        {
            ::pthread_mutex_init(&m_lock, NULL);
            ::pthread_mutex_init(&m_readLock, NULL);
            m_readRecord = 0;

            MapInfo_t mapinfo;
            mapinfo.cliCount = 16;
            mapinfo.MsgCount = 128;
            mapinfo.subMapSize = 2;

            m_map = NULL;
            m_map = ExpandMap(mapinfo);

        }

        int ThrRegister()
        {
            MapInfo_t * map = NULL;
            MapInfo_t mapInfo;


            cliIndex = -1;
            ClientSubMap_t * cli;
            ClientSubInfoIndex_t * cliSubInfo;
            unsigned int size = sizeof(ClientSubMap_t) + sizeof(ClientSubInfoIndex_t);

            unsigned int readRecord = m_readRecord;
            if (m_map == NULL) 
            {
                return -1;
            }

            pthread_mutex_lock(&m_lock);

            for (int iLoop = 0; iLoop < m_map->cliCount; iLoop++)
            {
                cli = (ClientSubMap_t *)((char *)m_map + sizeof(MapInfo_t) + size * iLoop);
                cliSubInfo = (ClientSubInfoIndex_t *)((char *)cli + sizeof(ClientSubMap_t);
                if (cli->st == 0)
                {
                    cliIndex = iLoop;
                    cli->st = 1;
                    pthread_mutex_unlock(&m_lock);
                    return cliIndex;
                }
            }

            //cli 数量不足 扩容
            cliIndex = m_map->cliCount;

            mapInfo.cliCount = m_map->cliCount + 16;
            mapInfo.msgCount = m_map->msgCount;
            mapInfo.subMapSize = mapInfo.cliCount / 8;

            map = ExpandMap(mapInfo);


            //设置开始写 读进入安全读模式
            m_IsWrite = 1;
            while (true) 
            {
                //等待处于非安全读下的操作完成 经验锁
                ::usleep(3000);
                if (readRecord == m_readRecord)
                {
                    break;
                }
                else
                {
                    readRecord = m_readRecord;
                }
            }
                
            //安全写
            ::pthread_mutex_lock(&m_readLock);
            ::free(m_map);
            m_map = map;
            ::pthread_mutex_unlock(&m_readLock);

            m_IsWrite = 0;

            pthread_mutex_unlock(&m_lock);
            return cliIndex;
        }

        int ThrUnRegister(int cliIndex)
        {
            return 0;
        }

        int MsgRegister(int msgID, int cliIndex, Function3<void(void *, unsigned int, void *)> & handleCB, Function3<void(void *, unsigned int, void *)> & errCB, void * userParam);
        {
            return 0;
        }

        int MsgUnRegister(int msgIndex, int cliIndex, int msgID)
        {
            return 0;
        }


        int QueryMsgSubMap(int msgIndex, int cliIndex, unsigned char * MsgSubCliMap, unsigned int sizeOfMap, unsigned int & lenOfMap)
        {
            if (m_IsWrite == 0)
            {
                m_readRecord++;
                m_nowMap = NULL;
            }
            else
            {
                //发生写 需要保证读安全
                ::pthread_mutex_lock(&m_readLock);
                m_nowMap = NULL;
                ::pthread_mutex_unlock(&m_readLock);
            }
            return 0;
        }

        int QueryMsgSubCliInfo(int msgIndex, int cliIndex, int msgID, ClientSubInfoIndex_t & subInfo)
        {
            return 0;
        }
    };
}
#endif
