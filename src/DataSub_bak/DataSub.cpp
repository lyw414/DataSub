#include "DataSub.h"

#include "ProClientTable.hpp"

namespace LYW_CODE
{
    DataSub::DataSub()
    {
        m_name = "";
        m_key = 0;
        m_param = NULL;
        m_cliIndex = -1;

        ::pthread_mutex_init(&m_lock, NULL);
    }



    bool DataSub::IsReadFinished(void * msg)
    {
        unsigned char * bitMap = (unsigned char *)msg + sizeof(int) * 2;
        int bitMapLen = *(int *)((unsigned char *)msg + sizeof(int));
        for (int iLoop = 0; iLoop < bitMapLen; iLoop++)
        {
            if (bitMap[iLoop] != 0x00)
            {
                return false;
            }
        }
        return true;
    }


    bool DataSub::IsNeedData(void * msg, unsigned int size, void * param)
    {
        unsigned char * bitMap = (unsigned char *)msg + sizeof(int) * 2;

        if (GetBit(bitMap, m_proID) == 0)
        {
            return false;
        }
        else
        {
            return true;
        }
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

            //拉起任务
            m_param->task = new TaskPool(threadNum);

            //拉起共享缓存 - IPC 
            m_param->ipc = new RWBlockArray_IPC(m_key);



            m_param->thSub = new ThreadSubTable();
            m_param->ipc->Destroy();
            m_param->ipc->Create(1024, 1024);
            m_param->ipc->Connect(Function1<bool(void *)>(&DataSub::IsReadFinished, this));


            //拉起进程订阅表 - ProSub
            m_param->proSub = new ProSubTable(m_key + 1);
            m_param->proSub->Destroy();
            m_param->proSub->Create();


            m_param->proSub->Connect();



            //拉起线程任务
            if (m_param->task->Start() < 0)
            {
                return -1;
            }


            //登记线程订阅信息
            ProClientTable::GetInstance()->SetParam(name, m_param );


            //拉起线程接收线程 
            ::pthread_attr_init(&m_attr);

            ::pthread_attr_setdetachstate(&m_attr, PTHREAD_CREATE_DETACHED);

            ::pthread_create(&m_thread, &m_attr, DataSub::RecvFromIPC_, this);


        }
        else
        {
            //等待服务就绪
            while(retry > 0)
            {
                //获取参数
                if ((m_param = (ShareInfo_t *)(ProClientTable::GetInstance()->GetParam(name))) != NULL)
                {
                    break;
                }
                retry--;
                usleep(500000);
            }
            m_param->ipc->Connect(Function1<bool(void *)>(&DataSub::IsReadFinished, this));

            m_param->proSub->Connect();


        }
        m_cliIndex = m_param->thSub->CliRegister();
        m_proID = m_param->proSub->ProRegister(getpid());
        return 0;
    }


    int DataSub::UnInit()
    {
        int res = 0;
        if (m_param != NULL)
        {
            m_param->thSub->CliUnRegister(m_cliIndex);
        }
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
            delete m_param->thSub;
            ::free(m_param);
            m_param = NULL;
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

        TaskPool::TaskNode_t taskNode;

        taskNode.handleFunc = Function2<void (void *, unsigned int)> (&DataSub::BlockHandle, this);

        taskNode.errFunc = NULL;

        int myID = 0;  //进程订阅表中获取id
        

        RWBlockArray_IPC::BlockHandle_t BH_t;
        RWBlockArray_IPC::BlockHandle_t * BH = &BH_t;
        Function3<bool(void *, unsigned int, void *)> IsNeed(&DataSub::IsNeedData, this);

        BH->index = 0;
        BH->id = 0;

        while(m_param->recvST != 2)
        {
            //获取进程订阅消息
            data = (char *)m_param->ipc->Read(BH, BH, IsNeed, &m_proID);
            taskNode.param = data;

            taskNode.lenOfParam = 128;

            //printf("ZZZZZZZZZZZZZZ %d %d\n", BH->id, BH->index);

            //添加块处理任务
            m_param->task->AddTask(taskNode);
            //BlockHandle(taskNode.param, taskNode.lenOfParam);
        }


        m_param->recvST = 0;
        return;
    }

    void DataSub::SetBit(unsigned char * bitMap, int index, unsigned char setValue)
    {
        unsigned char bit;
        unsigned int byteIndex = index / 8;
        unsigned int bitIndex = 7 - (index - byteIndex * 8);

        bit = 0x01;
        bit = bit << bitIndex;

        if (setValue == 0x00)
        {
            bitMap[byteIndex] &= (~bit);

        }
        else
        {
            bitMap[byteIndex] |= bit;
        }
    }
    
    int DataSub::GetBit(unsigned char * bitMap, int index)
    {
        unsigned char bit = 0x01;
        unsigned int byteIndex = index / 8;
        unsigned int bitIndex = 7 - (index - byteIndex * 8);
        bit = bit << bitIndex;
        
        if ((bitMap[byteIndex] & bit) == 0)
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }


    void DataSub::DoThreadHandleFunc(void * msg, unsigned int lenOfMsg)
    {
        int cliIndex = lenOfMsg;

        ThreadSubTable::SubInfo_t subInfo;
        
        int bitMapLen = *(int *)msg;
        unsigned char * bitMap = (unsigned char *)msg + sizeof(int);
        int msgIndex = *(int *)(bitMap + bitMapLen);

        m_param->thSub->QuerySubInfo(cliIndex, msgIndex, &subInfo);
        subInfo.handleCB((unsigned char *)msg + sizeof(int) * 2 + bitMapLen, 32, subInfo.userParam);
        
        ::pthread_mutex_lock(&m_lock);
        SetBit(bitMap, cliIndex, 0);
        for (int iLoop = 0; iLoop < bitMapLen; iLoop++)
        {
            if (bitMap[iLoop] != 0x00)
            {

            }
        }

        ::pthread_mutex_unlock(&m_lock);
    }

    void DataSub::BlockHandle(void * msg, unsigned int lenOfMsg)
    {
        //拷贝报文
        int msgID = *(int *)msg;

        int res = 0;

        int proBitMapLen = *(int *)((unsigned char *)msg + sizeof(int));

        //printf("PPPPPPPPPPPPPPPPP %d\n", lenOfMsg);
        TaskPool::TaskNode_t taskNode;

        taskNode.handleFunc = Function2<void (void *, unsigned int)> (&DataSub::DoThreadHandleFunc, this);

        //获取线程订阅信息长度
        int cliCount = m_param->thSub->CliCount();
        int bitMapLen = cliCount / 8;

        unsigned char * data = (unsigned char *)::malloc(lenOfMsg + bitMapLen - proBitMapLen);

        *(int *)data = bitMapLen;

        unsigned char * bitMap = data + sizeof(int);

        ::memset(data + sizeof(int), 0xFF, bitMapLen);

        ::memcpy(data + sizeof(int) * 2 + bitMapLen, (unsigned char *)msg + sizeof(int) + proBitMapLen, lenOfMsg - sizeof(int) - proBitMapLen );

        //读完成
        SetBit((unsigned char *)msg + sizeof(int) * 2, m_proID, 0);
        for (int iLoop = 0; iLoop < cliCount; iLoop++)
        {
            res = m_param->thSub->QueryMsgIsSub(iLoop, msgID);
            if (res < 0)
            {
                //设置无任务
                SetBit((unsigned char *)data + sizeof(int), iLoop, 0);
            }
        }

        //分配任务
        for (int iLoop = 0; iLoop < cliCount; iLoop++)
        {
            res = m_param->thSub->QueryMsgIsSub(iLoop, msgID);
            if (res < 0)
            {
                //设置无任务
                SetBit((unsigned char *)data + sizeof(int), iLoop, 0);
            }
            else
            {
                //执行任务
                *(int *)(data + sizeof(int) + bitMapLen) = res;
                taskNode.param = data;
                taskNode.lenOfParam = iLoop;
                m_param->task->AddTask(taskNode);
            }
        }


    }

    int DataSub::Publish(int msgID, void * msg, int lenOfMsg)
    {
        int msgIndex = m_param->thSub->GetProMsgMap(msgID);

        int len = 0;
        
        
        if (msgIndex == -1)
        {
            msgIndex = m_param->proSub->QueryMsgIndex(msgID);
            if (msgIndex < 0)
            {
                return -1;
            }

            m_param->thSub->SetProMsgMap(msgID, msgIndex);
        }


        m_param->proSub->QuerySubPro(msgIndex, msgID, (unsigned char *)msg + sizeof(int)*2, 32, len);

        
        *((int *)msg) = msgID;

        *((int *)((unsigned char *)msg + sizeof(int))) = len;

        m_param->ipc->Write(msg, lenOfMsg, 100);

        return 0;
    }

    int DataSub::Subcribe(int msgID, Function3<void(void *, unsigned int, void *)> handleCB, void * userParam, int mode, int timeout)
    {
        int proMsgIndex = m_param->thSub->GetProMsgMap(msgID);
        
        if (proMsgIndex < 0)
        {
            proMsgIndex = m_param->proSub->MsgRegister(msgID, m_proID, getpid() );
            m_param->thSub->MsgRegister(m_cliIndex, msgID, handleCB, NULL, userParam);
            m_param->thSub->SetProMsgMap(msgID, proMsgIndex);
        }
    }

}
