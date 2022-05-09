#include "DataSub.h"
#include "PublishSvr.h"
namespace LYW_CODE
{
    PublishTaskBase::PublishSvr::PublishSvr()
    {

    }

    PublishTaskBase::PublishSvr::~PublishSvr()
    {

    }

    PublishTaskBase::PublishSvr * PublishTaskBase::PublishSvr::GetInstance()
    {
        static PublishTaskBase::PublishSvr * instance  = new PublishSvr();
        return instance;
    }


    int PublishTaskBase::PublishSvr::Publish()
    {
        printf("publish data ....\n");
        return 0;
    }
    
}
