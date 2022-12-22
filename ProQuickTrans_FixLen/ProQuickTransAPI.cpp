#include "ProQuickTransAPI.h"
#include "ProQuickTrans.hpp"

#include "ProQuickTrans.CFG"

INTERFACE_API ProQuickTransHandle RM_CBB_ProQuickTransInit(key_t key)
{
    //解析g_proQuickTransCFG配置数组
    xint32_t count = sizeof(g_proQuickTransCFG) / (sizeof(xint32_t) * 4);

    RM_CODE::ProQuickTrans * handle = new RM_CODE::ProQuickTrans();
    xint32_t record = 0;

    for (xint32_t iLoop = 0; iLoop < count; iLoop++)
    {
        if (g_proQuickTransCFG[iLoop * 4] == key)
        {
            record++;
            handle->AddCFG(g_proQuickTransCFG[iLoop*4 + 1], g_proQuickTransCFG[iLoop*4 + 2], g_proQuickTransCFG[iLoop*4 + 3]);
        }
    }

    if (record > 0)
    {
        handle->Init(key);
    }
    else
    {
        delete handle;
        handle = NULL;
    }

    return handle;
}

INTERFACE_API void RM_CBB_ProQuickTransUnInit(ProQuickTransHandle * h)
{
    RM_CODE::ProQuickTrans ** handle = (RM_CODE::ProQuickTrans **)h;
    if ((*handle) != NULL)
    {
        (*handle)->UnInit();
        delete (*handle);
        *handle = NULL;
    }

    return;
}

INTERFACE_API void RM_CBB_ProQuickTransDestroy(key_t key)
{
    struct shmid_ds buf;
    xint32_t shmID = ::shmget(key, 0, IPC_EXCL);
    if (shmID >= 0)
    {
        ::shmctl(shmID, IPC_RMID, &buf);
    }
    return;
}

INTERFACE_API xint32_t RM_CBB_ProQuickTransWrite(ProQuickTransHandle h, xint32_t id, xbyte_t * data, xint32_t lenOfData, xint32_t interval, xint32_t timeout)
{
    RM_CODE::ProQuickTrans * handle = (RM_CODE::ProQuickTrans *)h;
    if (handle != NULL)
    {
        return handle->Write(id, data, lenOfData, interval, timeout);
    }
    return -3;
}

INTERFACE_API xint32_t RM_CBB_ProQuickTransRead(ProQuickTransHandle h, ProQuickTransNodeIndex_t * IN, xint32_t id, xbyte_t * data, xint32_t sizeOfData, xint32_t interval, xint32_t timeout)
{
    RM_CODE::ProQuickTrans * handle = (RM_CODE::ProQuickTrans *)h;
    if (handle != NULL)
    {
        return handle->Read(IN, id, data, sizeOfData, interval, timeout);
    }
    return -3;
}



INTERFACE_API xint32_t RM_CBB_ProQuickTransDataSize(ProQuickTransHandle h, xint32_t id)
{
    RM_CODE::ProQuickTrans * handle = (RM_CODE::ProQuickTrans *)h;
    if (handle != NULL)
    {
        return handle->DataSize(id);
    }
    return -1;
}

