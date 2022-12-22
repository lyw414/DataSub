#ifndef __RM_CODE_THREAD_SUB_TABLE_HPP_FILE__
#define __RM_CODE_THREAD_SUB_TABLE_HPP_FILE__


#include "streamaxcomdev.h"
#include "streamaxlog.h"
#include "SubTableDefine.h"

namespace RM_CODE {
    class ThreadSubTable {
        public:
            xint32_t Connect()
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

            xint32_t RegisterSubInfo(xint32_t msgID, DataSubInfo_t subInfo)
            {
                return 0;
            }

            xint32_t UnRegisterSubInfo(xint32_t msgHandle)
            {
                return 0;
            }
    };
};
#endif
