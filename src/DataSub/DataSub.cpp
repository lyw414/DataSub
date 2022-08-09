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



    bool DataSub::IsReadFinished(void * msg)
    {
        return false;
    }


    bool DataSub::IsNeedData(void * data, unsigned int size, void * param)
    {
        return false;
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

            //拉起线程接收线程 
            ::pthread_attr_init(&m_attr);

            ::pthread_attr_setdetachstate(&m_attr, PTHREAD_CREATE_DETACHED);

            ::pthread_create(&m_thread, &m_attr, DataSub::RecvFromIPC_, this);

            //拉起任务
            m_param->task = new TaskPool(threadNum);

            //拉起共享缓存 - IPC 
            m_param->ipc = new RWBlockArray_IPC(m_key);

            m_param->ipc->Create(1024, 1024);

            m_param->m_ipc->Connect(Funtion1<bool(void *)>(DataSub::IsReadFinished, this);

            //拉起进程订阅表 - ProSub
            m_param->proSub = new ProSubTable(m_key + 1);


            //拉起线程任务
            if (m_param->task->Start() < 0)
            {
                return -1;
            }

            //初始线程订阅表


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
        char * data = NULL;

        int myID = 0;  //进程订阅表中获取id
        
        //获取进程id
        int proID = m_param->proSub->ProRegister(pid());

        RWBlockArray_IPC::BlockHandle_t BH_t;
        RWBlockArray_IPC::BlockHandle_t * BH = &BH_t;
        Function3<bool(void *, unsigned int, void *) IsNeed(DataSub::IsNeedData, this);

        BH->index = 0;
        BH->id = 0;

        while(m_param->recvST != 2)
        {
            //获取进程订阅消息
            data = (char *)m_param->m_ipc->rwBlock.Read(BH, IsNeed, &proID);
            //添加块处理任务
            //m_param->task->AddTask( )
        }

        m_param->recvST = 0;
        return;
    }

}
