#include "ProQuickTransAPI.h"
#include "ProQuickTrans.hpp"

#include "ProQuickTrans.CFG"

INTERFACE_API ProQuickTransHandle RM_CBB_ProQuickTransInit(key_t key)
{
    //解析g_proQuickTransCFG配置数组
    RM_CODE::ProQuickTransCFG_t * cfg = NULL;
    xint32_t record = 0;
    xint32_t count = sizeof(g_proQuickTransCFG) / (sizeof(xint32_t) * 4);
    RM_CODE::ProQuickTrans * handle = new RM_CODE::ProQuickTrans();


    for (xint32_t iLoop = 0; iLoop < count; iLoop++)
    {
        if (g_proQuickTransCFG[iLoop * 3] == key)
        {
            record++;
        }
    }

    cfg = (RM_CODE::ProQuickTransCFG_t *)::malloc(sizeof(RM_CODE::ProQuickTransCFG_t) * record);

    for (xint32_t iLoop = 0; iLoop < count; iLoop++)
    {
        if (g_proQuickTransCFG[iLoop * 3] == key)
        {
            cfg[iLoop].id = g_proQuickTransCFG[iLoop*3 + 1];
            cfg[iLoop].size = g_proQuickTransCFG[iLoop*3 + 2];
        }
    }

    xint32_t ret = handle->Init(key, cfg, record);


    if (cfg != NULL)
    {
        ::free(cfg);
    }

    if (ret < 0)
    {
        if (handle != NULL)
        {
            delete handle;
            handle = NULL;
        }
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


INTERFACE_API xint32_t RM_CBB_ProQuickTransWrite(ProQuickTransHandle h, xint32_t id, xbyte_t * data, xint32_t lenOfData, xint32_t timeout, xint32_t spinInterval)
{
    RM_CODE::ProQuickTrans * handle = (RM_CODE::ProQuickTrans *)h;
    if (handle != NULL)
    {
        return handle->Write(id, data, lenOfData, timeout, spinInterval);
    }
    return -3;
}

INTERFACE_API xint32_t RM_CBB_ProQuickTransRead(ProQuickTransHandle h, xint32_t ID, ProQuickTransReadIndex_t * readIndex, void * data, xint32_t sizeOfData, xint32_t * outLen, ProQuickTransReadMode_e mode, xint32_t timeout, xint32_t spinInterval)
{
    RM_CODE::ProQuickTrans * handle = (RM_CODE::ProQuickTrans *)h;
    if (handle != NULL)
    {
        return handle->Read(ID, readIndex, data, sizeOfData, outLen, mode, timeout , spinInterval);
    }
    return -3;
}

