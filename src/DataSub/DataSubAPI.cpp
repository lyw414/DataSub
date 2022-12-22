#include "DataSubAPI.h"
#include "DataSubDefine.h"

extern "C" {
    INTERFACE_API xint32_t RM_CBB_DataSub_Init()
    {
        return 0;
    }
    INTERFACE_API xint32_t RM_CBB_DataSub_UnInit()
    {
        return 0;
    }
    INTERFACE_API xint32_t RM_CBB_DataSub_RegisterMSG(xint32_t msgID)
    {
        return 0;
    }
    INTERFACE_API xint32_t RM_CBB_DataSub_UnRegisterMSG(xint32_t msgID)
    {
        return 0;
    }
    INTERFACE_API xint32_t RM_CBB_DataSub_Publish(xint32_t msgHandle, void * msg, xint32_t lenOfMsg, xint32_t timeout)
    {
        return 0;
    }
    INTERFACE_API xint32_t RM_CBB_DataSub_Subcribe(xint32_t msgHandle, RM_CODE::Function3<void(void *, xint32_t,  void *)> handleFunc, RM_CODE::Function3<void(void *, xint32_t, void *)> errFunc, void * userParam)
    {
        return 0;
    }
}

