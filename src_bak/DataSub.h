#ifndef __LYW_CODE_DATA_SUB_H__
#define __LYW_CODE_DATA_SUB_H__ 0
#include <stdio.h>
#include <string>
namespace LYW_CODE
{
    extern "C" int LYW_CODE_SUB_IPC_SET(const char * id = "LYW_CODE_DATA_SUB", unsigned int dataBufferCacheSize = 1024 * 1024 * 2);

    extern "C" int LYW_CODE_SUB_SVR_TASK_SET(unsigned int maxThreadNum = 8);

    class PublishTaskBase
    {
    private:
        class PublishSvr;
        friend class PublishSvr;
        PublishSvr * m_pub;
    public:
        PublishTaskBase();
        virtual ~PublishTaskBase();
        int Publish(unsigned int dataType, const std::string & data, const unsigned int & timeout = 100);

    };

    class SubscribeTaskBase
    {
    private:
        class SubscribeSvr;
        friend class SubscribeSvr;
        SubscribeSvr * m_sub;

    public:
        SubscribeTaskBase();

        virtual ~SubscribeTaskBase();

        virtual int subTask(std::string & data);

        virtual int subTaskTimeout();

        virtual int timeTask();

        virtual int timeTaskTimeout();

        void registerSubTask(unsigned int dataType);

        void unregisterSubTask(unsigned int dataType);

        void registerTimeTask(unsigned int interval);

        void unregisterTimeTask(unsigned int interval);

    };
}
#endif
