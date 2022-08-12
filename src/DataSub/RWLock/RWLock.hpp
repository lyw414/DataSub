#ifndef __LYW_CODE_READ_WRITE_LOCK_HPP__
#define __LYW_CODE_READ_WRITE_LOCK_HPP__

#include <pthread.h>
#include <unistd.h>

namespace LYW_CODE 
{
    class RWDelaySafeLock {
    private:
        // 0 无锁读 1 加锁读
        int m_writeTag;
        int m_readRecord;
        pthread_mutex_t m_lock;
    
    public:
        RWDelaySafeLock()
        {
            ::pthread_mutex_init(&m_lock, NULL);
            m_writeTag = 0;
            m_readRecord = 0;
        }

        ~RWDelaySafeLock()
        {
            ::pthread_mutex_destroy(&m_lock);
        }


        void WriteLock(unsigned long delay)
        {
            unsigned int readRecord = m_readRecord;
            m_writeTag = 1;
            while(true)
            {
                ::usleep(delay);
                if (readRecord == m_readRecord)
                {
                    break;
                }
                else
                {
                    readRecord = m_readRecord;
                }
            }
            ::pthread_mutex_lock(&m_lock);
        }


        void WriteUnLock()
        {
            m_writeTag = 0;
            ::pthread_mutex_unlock(&m_lock);
        }

        int ReadLock()
        {
            if (m_writeTag == 0)
            {
                m_readRecord++;
                return 0;
            }
            else
            {
                //锁
                ::pthread_mutex_lock(&m_lock);
                return 1;
            }
        }

        void ReadUnLock(int st)
        {
            if (st == 1)
            {
                ::pthread_mutex_unlock(&m_lock);
            }
        }
    };
}
#endif

