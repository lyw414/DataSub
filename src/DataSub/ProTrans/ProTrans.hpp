#ifndef __RM_CODE_PRO_TRONS_HPP_FILE
#define __RM_CODE_PRO_TRONS_HPP_FILE
#include <sys/types.h>

#include "streamaxcomdev.h"
#include "streamaxlog.h"
#include "ProTransDefine.h"
#include "SimpleFunction.hpp"

namespace RM_CODE
{
    class ProTrans {
        private:
            key_t m_key;

        public:
            ProTrans()
            {
                m_key = -1;
            }

            ~ProTrans()
            {
            }
            xint32_t Connect(key_t key, xint32_t blockNum, xint32_t blockSize, Function2<bool(xbyte_t *, xint32_t)> & isFinishedRead )
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

            xint32_t Write(void * msg, xint32_t lenOfMsg)
            {
                return 0;
            }

            xint32_t BlockSize()
            {
                return 0;
            }
            
            xbyte_t * Read(ProTransReadIndex_t * rdIndex, Function2<bool(xbyte_t *, xint32_t)> & isNeed, xint32_t timeout)
            {
                return NULL;
            }

    };
}
#endif
