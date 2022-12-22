#ifndef __RM_CODE_SUB_TABLE_HPP_FILE__
#define __RM_CODE_SUB_TABLE_HPP_FILE__

#include <sys/types.h>
#include "streamaxcomdev.h"
#include "streamaxlog.h"

#include "SubTableDefine.h"

namespace RM_CODE {

    class SubTable {

        public:
            xint32_t Connect(key_t key)
            {
                return 0;
            }

            xint32_t DisConnect()
            {
                return 0;
            }

            xint32_t Destroy()
            {
                return 0;
            }

            xint32_t MsgRegister(xint32_t msgID)
            {
                return 0;
            }

            xint32_t MsgUnRegister(xint32_t msgHandle)
            {
                return 0;
            }

            xint32_t ProSubTable(xint32_t msgHandle, void * subTable, xint32_t sizeOfTable, xint32_t  * lenOfTable)
            {
                return 0;
            }

            xint32_t ThrSubTable(xint32_t msgIndex, void * subTable, xint32_t sizeOfTable, xint32_t * lenOfTable)
            {
                return 0;
            }

            xint32_t SetSubInfo(xint32_t msgHandle, const DataSubInfo_t * subInfo)
            {
                return 0;
            }

            xint32_t GetSubInfo(xint32_t msgHandle, DataSubInfo_t * subInfo)
            {
                return 0;
            }
    };
}
#endif
