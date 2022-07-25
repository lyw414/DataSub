#ifndef __LYW_CODE_PRO_MEMORY_HPP_FILE_
#define __LYW_CODE_PRO_MEMORY_HPP_FILE_
#include <pthread.h>
#include <stdlib.h>

#include "MemBase.hpp"

namespace LYW_CODE
{
    class ProMemory: MemoryBase
    {
    private:
        pthread_mutex_t m_lock;

    public:
        ProMemory()
        {
            ::pthread_mutex_init(&m_lock, NULL);
        }

        virtual ~ProMemory()
        {
            ::pthread_mutex_destroy(&m_lock);
        }

        virtual int IsInit(void * ptr)
        {
            return 0;
        }

        virtual void * malloc(int size)
        {
            return ::malloc(size);
        }

        virtual void free(void * ptr)
        {
            ::free(ptr);
        }

        virtual void Lock()
        {
            ::pthread_mutex_lock(m_lock);
        }

        virtual void UnLock()
        {
            ::pthread_mutex_unlock(m_lock);
        }
    };
}
#endif
