#ifndef __LYW_CODE_THREAD_SUB_TABLE_HPP_
#define __LYW_CODE_THREAD_SUB_TABLE_HPP_

#include "DataSubFunc.hpp"

namespace LYW_CODE
{
    //数据结构为 (ClientSubMap_t + ClientSubInfo_t)[] + MsgSubMap_t[]
    class ThreadSubTable
    {
    public:
        typedef _ClientSubInfo {
            void * userParam;
            Function3<void(void *, unsigned int, void *)> handleCB;
            Function3<void(void *, unsigned int, void *)> ErrorCB;
        } ClientSubInfo_t;

    private:
        typedef struct _ClientSubMap {
            int MsgSubBitMapSize;
            unsigned char MsgSubBitMap[0];
        } ClientSubMap_t;


        typedef struct _MsgSubInfo {
            int msgID;
            int ClientSubBitMapSize;
            unsigned char ClientSubBitMap[0];
        } MsgSubMap_t;

    private:

    public:
        int ThrRegister()
        {
            return 0;
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
            return 0;
        }

        int QueryMsgSubCliInfo(int msgIndex, int cliIndex, ClientSubInfo_t & subInfo)
        {
            return 0;
        }

    };
}
#endif
