#ifndef __RM_CODE_PRO_INSTANCE_HPP_FILE_
#define __RM_CODE_PRO_INSTANCE_HPP_FILE_
#include <map>
#include <string>
#include <pthread.h>

#include "streamaxcomdev.h"
#include "streamaxlog.h"


namespace RM_CODE {
    class ProInstance {
        private:
            std::map<std::string, pid_t> m_map;
            pid_t m_pid;
            ::pthread_mutex_t m_lock;

        public:
            ProInstance()
            {
                m_pid = getpid();
                ::pthread_mutex_init(&m_lock, NULL);
                m_map.clear();
            }


            static ProInstance * GetInstance()
            {
                static ProInstance * instance = new ProInstance();
                return instance;
            }

            xint32_t RegisterID(const std::string ID)
            {
                xint32_t ret = 0;
                ::pthread_mutex_lock(&m_lock);
                if (m_pid != getpid())
                {
                    m_pid = getpid();
                    m_map.clear();
                }

                if (m_map.find(ID) != m_map.end())
                {
                    ret = 1;
                }
                else
                {
                    m_map[ID] = m_pid;
                    ret = 0;
                }
                ::pthread_mutex_unlock(&m_lock);
                return 0;
            }


            xint32_t UnRegisterID(const std::string ID)
            {
                xint32_t ret = 0;
                ::pthread_mutex_lock(&m_lock);
                if (m_pid != getpid())
                {
                    m_pid = getpid();
                    m_map.clear();
                }

                m_map.erase(ID);
                ::pthread_mutex_unlock(&m_lock);
                return 0;
            }

    };
}
#endif
