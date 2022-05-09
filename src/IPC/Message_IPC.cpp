#include "Message_IPC.h"
namespace LYW_CODE
{
    Message_IPC::Message_IPC(const char * IPC_ID)
    {
        m_st = 0;

        m_shmID = 0;

        m_blockCount = 1024;

        m_blockSize = 1024;

        //m_timeout = 500;
        m_timeout = 0;

        m_infoID = 0;

        m_ipcStrID = (char *)IPC_ID;
    }

    Message_IPC::~Message_IPC()
    {
        m_st = 0;

        m_shmID = 0;

    }

    int Message_IPC::ReCreate(unsigned int maxDataBlockSize, unsigned int maxDataBlockCount, unsigned int timeout)
    {
        return -1;
    }

    int Message_IPC::Connect()
    {
        int res = 0;

        size_t len = 0;

        if (m_st == 1)
        {
            return m_shmID;
        }

        //connect to shm
        len = sizeof(IPC_INFO_SHM_t) + (sizeof(MessageHead_t) + m_blockSize) * m_blockCount;

        //create shm
        m_shmID = ::shmget(0x1A, len, 0666 | IPC_CREAT | IPC_EXCL);

        if (m_shmID >= 0)
        {
            //create by you do init
            m_shmInfo = (IPC_INFO_SHM_t *)::shmat(m_shmID, NULL, 0);
            ::memset(m_shmInfo, 0x00, len);

            m_shmInfo->check = 0x3A;

            m_shmInfo->infoID = 1;

            m_shmInfo->st = 0x3A;

            m_shmInfo->timeout = m_timeout;

            m_shmInfo->queque.total = m_blockCount ;

            m_shmInfo->queque.begin = 0;

            m_shmInfo->queque.end = 1;

            m_shmInfo->queque.blockSize = sizeof(MessageHead_t) + m_blockSize;

            m_shmInfo->queque.dataSize = m_blockSize;

            //mutex
            pthread_mutexattr_init(&m_shmInfo->attrLock);

            pthread_mutexattr_setpshared(&m_shmInfo->attrLock, PTHREAD_PROCESS_SHARED);
            
            pthread_mutex_init(&m_shmInfo->lock, &m_shmInfo->attrLock);
            //cond 
            pthread_condattr_init(&m_shmInfo->attrCond);

            pthread_condattr_setpshared(&m_shmInfo->attrCond, PTHREAD_PROCESS_SHARED);
            
            pthread_cond_init(&m_shmInfo->cond, &m_shmInfo->attrCond);
        }
        else
        {
            //ipc exist just attatch and check
            m_shmID = ::shmget(0x1A, 0, 0);

            m_shmInfo = (IPC_INFO_SHM_t *)::shmat(m_shmID, NULL, 0);
            //do check and wait init finished
        }

        printf("XX%d::%d\n", getpid(), m_shmID);

        return 0;
    }

    int Message_IPC::Publish(MessageID_e msgid, unsigned char * data, unsigned int lenOfData, unsigned int timeout)
    {

        return 0;
    }

    int Message_IPC::Subcribe(MessageID_e msgid, unsigned char * data, unsigned int lenOfData, unsigned int timeout)
    {
        return 0;
    }

    int Message_IPC::Disconnect()
    {
        return 0;
    }

    int Message_IPC::IsReady()
    {
        return 0;
    }

    void Message_IPC::Lock()
    {
        ::pthread_mutex_lock(&m_shmInfo->lock);
    }

    void Message_IPC::Unlock()
    {
        ::pthread_mutex_unlock(&m_shmInfo->lock);
    }

    void Message_IPC::Test()
    {
        printf("%d::%d\n",getpid(),m_shmInfo->timeout++);
    }
}
