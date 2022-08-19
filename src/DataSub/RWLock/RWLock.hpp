#ifndef __LYW_CODE_READ_WRITE_LOCK_HPP__
#define __LYW_CODE_READ_WRITE_LOCK_HPP__

#include <unistd.h>

namespace LYW_CODE 
{
    class RWDelaySafeLock {
    private:
        // 0 无锁读 1 加锁读
        volatile int m_writeTag;
        volatile int m_readRecord;
        volatile int m_readRecord1;
    
    public:
        RWDelaySafeLock()
        {
            m_writeTag = 0;
            m_readRecord = 0;
            m_readRecord1 = 0;
        }

        ~RWDelaySafeLock()
        {
        }


        inline void WriteLock(unsigned long delay)
        {
            unsigned int record = m_readRecord;
            m_writeTag = 1;
            while(true)
            {
                ::usleep(delay);
                if (m_readRecord == record)
                {
                    break;
                }
                else
                {
                    m_readRecord = record;
                }

            }
        }


        inline void WriteUnLock(unsigned long delay)
        {
            unsigned int record = m_readRecord1;
            m_writeTag = 0;
            while(true)
            {
                ::usleep(delay);
                if (m_readRecord1 == record)
                {
                    break;
                }
                else
                {
                    m_readRecord1 = record;
                }
            }

        }

        inline int ReadLock()
        {
            if (m_writeTag == 0)
            {
                m_readRecord++;
                return 0;
            }
            else
            {
                m_readRecord1++;
                return 1;
            }
        }

        inline int ReadUnLock(int st)
        {
            return 0;
        }
    };
}
#endif

