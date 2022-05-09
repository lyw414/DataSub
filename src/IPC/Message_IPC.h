#ifndef __LYW_CODE_MESSAGE_IPC_H__
#define __LYW_CODE_MESSAGE_IPC_H__

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define LYW_CODE_SUB_INFO_MAX_SIZE 8

namespace LYW_CODE
{
    typedef enum _MessageID 
    {
        MSG_TIME_OUT = 0,
        IPC_DESTORY_OUT,
        IPC_RECREATE_OUT,
        DATA_XX = 1024,
        MSG_SIZE,
    } MessageID_e;

    typedef struct _MessageHead
    {
        unsigned char subInfo[LYW_CODE_SUB_INFO_MAX_SIZE];   //订阅者信息 进程唯一
        unsigned int timeCount;     //计时器 处理超时 精度ms
        unsigned short msgIndex;    //消息序号
        unsigned short msgID;       //消息序号
        unsigned int packageCount;  //拆包总数 0 独立包
        unsigned int packageIndex;  //拆包序列号
        unsigned int dataLen;       //数据长度 可设置最大值
        unsigned char data[0];      //数据指针
    } MessageHead_t;

    typedef struct _IPC_MESSAGE_QUEUE_SHM 
    {
        //begin block index
        unsigned int begin;  
        
        //end block index
        unsigned int end;  
        
        //total block Size
        unsigned int total;  
        
        //block size = MessageHead_t + dataSize
        unsigned int blockSize;  
        
        //data size
        unsigned int dataSize;  

        //block begin address
        unsigned char block[0];
    } IPC_MESSAGE_QUEUE_SHM_t ;

    typedef struct _IPC_INFO_SHM 
    {
        //fixed 0x3A
        unsigned char check;

        //1 ready  others not ready
        unsigned char st;

        //info will may be chanaged check infoID before use it
        unsigned short infoID;
        
        //time out ms
        unsigned int timeout;
    
        //进程锁与条件遍历存在操作系统不支持风险 替换方案为信号量 进程锁性能优于信号量
        pthread_mutex_t lock;

        pthread_mutexattr_t attrLock;

        pthread_cond_t cond;

        pthread_condattr_t attrCond;

        //progress register info progress_register_info[id][0] pid progress_register_info[id][1] refrenceCount
        pid_t progress_register_info[LYW_CODE_SUB_INFO_MAX_SIZE * 8][2];
        //msg register info
        unsigned char message_register_info[MSG_SIZE][LYW_CODE_SUB_INFO_MAX_SIZE];

        //message queue
        IPC_MESSAGE_QUEUE_SHM_t queque;

    } IPC_INFO_SHM_t;

    /**
     * @brief           单例 进程唯一通讯ID
     */
    class Message_IPC
    {
    private:
        //ipc state
        unsigned int m_st;
        
        //ipc id
        int m_shmID;
        
        std::string m_ipcStrID;
        
        unsigned int m_blockCount;

        unsigned int m_blockSize;

        unsigned int m_timeout;

        unsigned int m_infoID;

        IPC_INFO_SHM_t * m_shmInfo;

    private:
        int Init();


    public:
        
        void Lock();

        void Unlock();

    public:
        Message_IPC(const char * IPC_ID = "ProMessageIPC");

        ~Message_IPC();
        
        int Connect();

        int ReCreate(unsigned int maxDataBlockSize = 1024, unsigned int maxDataBlockCount= 1024, unsigned int timeout = 1000);

        int IsReady();

        int Publish(MessageID_e msgid, unsigned char * data, unsigned int lenOfData, unsigned int timeout);

        int Subcribe(MessageID_e msgid, unsigned char * data, unsigned int lenOfData, unsigned int timeout);

        int Disconnect();

        void Test();
    };
};

#endif
