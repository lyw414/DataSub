#include "DataSub.h"
#include "SubscribeSvr.h"
namespace LYW_CODE
{
    SubscribeTaskBase::SubscribeTaskBase()
    {
        m_sub = LYW_CODE::SubscribeTaskBase::SubscribeSvr::GetInstance();
    }

    SubscribeTaskBase::~SubscribeTaskBase()
    {

    }

    int SubscribeTaskBase::subTask(std::string & data)
    {
        //无效任务
        return -1;
    }

    int SubscribeTaskBase::timeTask()
    {
        //无效任务
        return -1;
    }

    void SubscribeTaskBase::registerSubTask(unsigned int dataType)
    {
        m_sub->registerSubTask(this);
        printf("XXXXXXXXXXXX %p\n", this);
        return;
    }
    
    void SubscribeTaskBase::unregisterSubTask(unsigned int dataType)
    {
        return;
    }
 
    void SubscribeTaskBase::registerTimeTask(unsigned int dataType)
    {
        return;
    }
    
    void SubscribeTaskBase::unregisterTimeTask(unsigned int dataType)
    {
        return;
    }

    int SubscribeTaskBase::subTaskTimeout()
    {
        return 0;
    }

    int SubscribeTaskBase::timeTaskTimeout()
    {
        return 0;
    }
}
