#include "DataSub.h"

#include "ProClientTable.hpp"

namespace LYW_CODE
{
    DataSub::DataSub()
    {
        m_name = "";
        m_key = 0;
        m_param = NULL;
    }

    int DataSub::Init(const char * name, int threadNum)
    {
        int res = 0;

        int retry = 10;

        if (m_param != NULL)
        {
            return -1;
        }

        if ((res = ProClientTable::GetInstance()->Register(name, m_key)) >= 0)
        {
            m_name = std::string(name);
        }
        else
        {
            m_param = NULL;
            return -1;
        }
        
        if (res == 0)
        {
            //需要拉起服务
            m_param = (ShareInfo_t *)::malloc(sizeof(ShareInfo_t));

            //创建接收线程阻塞监听进程消息
            ::pthread_attr_init(&m_attr);

            ::pthread_attr_setdetachstate(&m_attr, PTHREAD_CREATE_DETACHED);

            ::pthread_create(&m_thread, &m_attr, DataSub::RecvFromIPC_, this);

            //拉起任务
            m_param->task = new TaskPool(threadNum);

            //拉起共享缓存

            if (m_param->task->Start() < 0)
            {
                return -1;
            }

            ProClientTable::GetInstance()->SetParam(name, m_param );
            return 0;
        }
        else
        {
            //等待服务就绪
            while(retry > 0)
            {
                //获取参数
                if ((m_param = (ShareInfo_t *)(ProClientTable::GetInstance()->GetParam(name))) != NULL)
                {
                    return 0;
                }
                retry--;
                usleep(500000);
            }

            return -1;
        }
    }

    int DataSub::UnInit()
    {
        int res = 0;
        //最后一个注销的线程 需要终止服务
        if ((res = ProClientTable::GetInstance()->UnRegister(m_name.c_str())) == 0)
        {
            //是否资源
            m_param->recvST = 2;
            while (m_param->recvST != 0)
            {
                ::usleep(50000);
            }
            m_param->task->Stop();
            delete m_param->task;
            ::free(m_param);
        }
        return 0;
    }

    void * DataSub::RecvFromIPC_ (void * ptr)
    {
        DataSub * self = (DataSub *)ptr;

        if (self != NULL)
        {
            self->RecvFromIPC();
        }
        return NULL;
    }

    void DataSub::RecvFromIPC()
    {
        while(m_param->recvST != 2)
        {
            //IPC 接收消息 并添加任务
        }
        m_param->recvST != 0;
        return;
    }

}
