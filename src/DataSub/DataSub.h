#ifndef __LYW_CODE_DATA_SUB_H_FILE__
#define __LYW_CODE_DATA_SUB_H_FILE__

#include "DataSubFunc.hpp"
#include <string>

namespace LYW_CODE
{
    class DataSub
    {
    private:
        key_t m_key;

        std::string m_name;

        pthread_mutex_t m_lock;

        TaskPool m_task;

    private:


    public:
        DataSub();

        int Init(const char * name);

        int UnInit();

        int Publish(int msgID, void * msg, int lenOfMsg);

        int Subcribe(int msgID, int HandleMsg, int mode, int timeout);

        int Response(int publishInfo, void * msg, int lenOfMsg);

        int SubcribeResponse(int msgID, int HandleMsg, int timeout);

        int UnSubcribe(int msgID);

        int UnSubcribeResponse(int msgID);
    };

}
#endif
