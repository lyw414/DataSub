#include "DataSub.h"
#include "PublishSvr.h"
namespace LYW_CODE
{
    PublishTaskBase::PublishTaskBase()
    {
        m_pub = LYW_CODE::PublishTaskBase::PublishSvr::GetInstance();
    }

    PublishTaskBase::~PublishTaskBase()
    {

    }

    int PublishTaskBase::Publish(unsigned int dataType, const std::string & data, const unsigned int & timeout)
    {
        printf("publish data 0 ....\n");
        m_pub->Publish();
    }
    
}
