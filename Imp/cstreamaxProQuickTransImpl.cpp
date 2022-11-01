#include "cstreamaxProQuickTransImpl.h"
namespace RM_CODE
{
    ProQuickTransImp::ProQuickTransImp()
    {
        RM_CBB_TaskPoolCreate(128,2);
        m_proQuickTransHandle = RM_CBB_ProQuickTransInit(0xAA);
        ::pthread_mutex_init(&m_lock, NULL);
    }

    ProQuickTransImp * ProQuickTransImp::GetInstance()
    {
        static ProQuickTransImp * ptr = new ProQuickTransImp;
	    RM_CBB_LOG_ERROR("STREAMAX.SDK", "%d GetInstance %p\n", getpid(), ptr);
        return ptr;
    }



    void ProQuickTransImp::TransTimeReadTask(void * param, xint32_t lenOfParam, void * userParam)
    {
        TransParam_t * transParam = (TransParam_t *)userParam;
        xint32_t len = 0;

        len = RM_CBB_ProQuickTransRead(m_proQuickTransHandle, &(transParam->IN), transParam->dataType, transParam->readData, transParam->sizeOfData, 10000, 100000);

	    RM_CBB_LOG_ERROR("STREAMAX.SDK", "ZZZZZZZZ %d dataType %d  func %p data0 %p data1 %p len[1:%d 0:%d] recvLen %d\n", getpid(), transParam->dataType, transParam->func, transParam->data0, transParam->data1, sizeof(GpsInfo_t), sizeof(GpsAgreementData_t), len);

        if (len > 0)
        {
            //GpsInfo_t * gps = (GpsInfo_t *)(transParam->dataType);
	        //RM_CBB_LOG_ERROR("STREAMAX.SDK", "DDDDDDDD st %c type %c planetNum %c\n", gps->gpsStatus, gps->gpsType, gps->gpsPlanetNum);
            transParam->func(transParam->readData, len, transParam->data0, transParam->data1);

        }
    }


    xint32_t ProQuickTransImp::RegisterTransTask(AsyncCallbackFunc func, xpvoid_t data0, xpvoid_t data1, xuint32_t dataType, xuint32_t interval)
    {
        TransParamKey_t key;
        TransParam_t * param;
        xint32_t sizeOfData = 0;
        key.data0 = data0;
        key.data1 = data1;
        

	    RM_CBB_LOG_ERROR("STREAMAX.SDK", "XXXXXXXXXXXXXXX %d %p %d func %p\n", sizeOfData, m_proQuickTransHandle, dataType, func);

        sizeOfData = RM_CBB_ProQuickTransDataSize(m_proQuickTransHandle, dataType);

        if (sizeOfData <= 0)
        {
            return -1;
        }

        TaskPoolCB_t cb(&ProQuickTransImp::TransTimeReadTask,this);

        ::pthread_mutex_lock(&m_lock);
        if (m_transParam.find(key) != m_transParam.end())
        {
            //更新
            param = m_transParam[key];
            RM_CBB_UnRegisterTask(param->taskHandle);

            if (func != NULL)
            {
                param->func = func;
            }
            else
            {

                delete param->readData;
                delete param;
            }

        }
        else if (func != NULL)
        {
            param = new TransParam_t;
            param->func = func;
            param->data0 = data0;
            param->data1 = data1;
            param->dataType = dataType;
            param->IN.mode = 0;
            param->IN.index = -1;
            param->taskHandle = NULL;
            param->sizeOfData = sizeOfData;
            param->readData = new xbyte_t[sizeOfData];
        }

        if (func != NULL)
        {
            m_transParam[key] = param;

            switch(dataType)
            {
            case 1:
                //根据不同的数据类型 适配任务模式
                param->taskHandle = RM_CBB_RegisterTimeTask(cb, NULL, param, interval, true, 5);
                break;
            case 0:
                //GPS
                param->taskHandle = RM_CBB_RegisterTimeTask(cb, NULL, param, interval, true, 5);
            default:
                break;
            }
        }
        ::pthread_mutex_unlock(&m_lock);
        return 0;
    }
}

