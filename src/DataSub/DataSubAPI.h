#ifndef __RM_CODE_DATA_SUB_API_H_FILE__
#define __RM_CODE_DATA_SUB_API_H_FILE__

#include <sys/types.h>

#include "streamaxcomdev.h"
#include "SimpleFunction.hpp"
extern "C" {
    INTERFACE_API xint32_t RM_CBB_DataSub_Init();
    INTERFACE_API xint32_t RM_CBB_DataSub_UnInit();
    INTERFACE_API xint32_t RM_CBB_DataSub_RegisterMSG(xint32_t msgID);
    INTERFACE_API xint32_t RM_CBB_DataSub_UnRegisterMSG(xint32_t msgID);
    INTERFACE_API xint32_t RM_CBB_DataSub_Publish(xint32_t msgHandle, void * msg, xint32_t lenOfMsg, xint32_t timeout);
    INTERFACE_API xint32_t RM_CBB_DataSub_Subcribe(xint32_t msgHandle, RM_CODE::Function3<void(void *, xint32_t,  void *)> handleFunc, RM_CODE::Function3<void(void *, xint32_t, void *)> errFunc, void * userParam);
}
#endif
