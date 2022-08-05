#ifndef __LYW_CODE_PROCLIENTTABLE_HPP__
#define __LYW_CODE_PROCLIENTTABLE_HPP__

#include <map>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <stdio.h>

#include "Hash.hpp"

namespace LYW_CODE
{
    class ProClientTable 
    {
    private:
        typedef struct _Client {
            int count;
            char name[256];
            key_t key;
        } Client_t;
        
        pthread_mutex_t m_lock;

        std::map<std::string, Client_t> m_map;

        ProClientTable() 
        {
            ::pthread_mutex_init(&m_lock, NULL);
            m_map.clear();
        };

    public:
        
        static ProClientTable * GetInstance()
        {
            static ProClientTable * self = new ProClientTable();

            return self;
        }

        int Register(const char * name, key_t & key)
        {
            char tmp[512] = { 0 };

            Client_t client = { 0 };

            std::string keyStr;

            key = Hash::APHash(name, ::strlen(name));
            
            ::memset(tmp, 0x00, 512);

            snprintf(tmp, 512, "%d_%s", key, name);

            keyStr = std::string(tmp);

            ::pthread_mutex_lock(&m_lock);
            if (m_map.find(keyStr) != m_map.end())
            {
                if (::strcmp(m_map[keyStr].name, name) == 0)
                {
                    m_map[keyStr].count++;
                    ::pthread_mutex_unlock(&m_lock);
                    return 0;
                }
                else
                {
                    //键值冲突
                    ::pthread_mutex_unlock(&m_lock);
                    return -1;
                }
            }
            else
            {
                ::strcpy(client.name, name);
                client.count = 1;
                client.key = key;
                m_map[keyStr] = client;
                ::pthread_mutex_unlock(&m_lock);
                return 0;
            }

            return 0;
        }
        
        void UnRegister(const char * name)
        {
            char tmp[512] = { 0 };

            std::string keyStr;

            key_t key = Hash::APHash(name, ::strlen(name));
            
            ::memset(tmp, 0x00, 512);

            snprintf(tmp, 512, "%d_%s", key, name);

            keyStr = std::string(tmp);

            ::pthread_mutex_lock(&m_lock);
            if (m_map.find(keyStr) != m_map.end())
            {
                if (::strcmp(m_map[keyStr].name, name) == 0)
                {
                    m_map[keyStr].count--;
                    if (m_map[keyStr].count == 0)
                    {
                        m_map.erase(keyStr);
                    }
                    ::pthread_mutex_unlock(&m_lock);
                    return;
                }
            }
            ::pthread_mutex_unlock(&m_lock);
        }
    };
}
#endif
