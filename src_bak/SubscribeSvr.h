#ifndef __LYW_CODE_SUBCRIBE_SVR__
#define __LYW_CODE_SUBCRIBE_SVR__ 0
#include "IPC.h"
#include "DataSub.h"
namespace LYW_CODE
{
    class SubscribeTaskBase::SubscribeSvr
    {
    private :
        IPC * m_ipc;
        SubscribeSvr();
    public:
        ~SubscribeSvr();

        static SubscribeTaskBase::SubscribeSvr * GetInstance();

        int registerSubTask(SubscribeTaskBase * subTask);

        int registerTimeTask(SubscribeTaskBase * timeTask);

    };
}
#endif
