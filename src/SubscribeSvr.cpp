#include "SubscribeSvr.h"
namespace LYW_CODE
{
    SubscribeTaskBase::SubscribeSvr::SubscribeSvr()
    {

    }

    SubscribeTaskBase::SubscribeSvr::~SubscribeSvr()
    {

    }

    SubscribeTaskBase::SubscribeSvr * SubscribeTaskBase::SubscribeSvr::GetInstance()
    {
        static  SubscribeSvr * instance = new SubscribeSvr();
        return  instance;
    }


    int SubscribeTaskBase::SubscribeSvr::registerSubTask(SubscribeTaskBase * subTask)
    {
        printf("ZZZZZZZZZZ %p %p %p\n", subTask, subTask->m_sub, this);
        return 0;
    }

    int SubscribeTaskBase::SubscribeSvr::registerTimeTask(SubscribeTaskBase * timeTask)
    {
        return 0;
    }

}
