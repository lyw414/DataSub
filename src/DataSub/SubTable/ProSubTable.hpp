#ifndef __RM_CODE_PRO_SUB_TABLE_H_FILE__
#define __RM_CODE_PRO_SUB_TABLE_H_FILE__

#include <sys/types.h>
#include "streamaxcomdev.h"
#include "streamaxlog.h"

namespace RM_CODE 
{
    class ProSubTable {
        private:

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

            xint32_t Destroy(key_t key)
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

    };
}
#endif
