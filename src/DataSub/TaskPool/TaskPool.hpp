#ifndef __RM_CODE_TASK_POOL_HPP_FILE__
#define __RM_CODE_TASK_POOL_HPP_FILE__
#include <string>

#include "streamaxcomdev.h"
#include "streamaxlog.h"
#include "SimpleFunction.hpp"

namespace RM_CODE {
    class TaskPool {
        public:
            xint32_t Connect(const std::string & ID, xint32_t threadNum)
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

            
            xint32_t Register()
            {
                return 0;
            }

            xint32_t UnRegister()
            {
                return 0;
            }

            xint32_t SetTaskAttr(xint32_t taskHandle)
            {
                return 0;
            }

            xint32_t ExcuteTask(xint32_t taskHandle, Function3<void(void *, xint32_t, void *)> handleFunc, void * data, xint32_t lenOfData, void * userParam)
            {
                return 0;
            }
    };
}

#endif
