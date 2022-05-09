#ifndef __LYW_CODE_PUBLISH_SVR_H__
#define __LYW_CODE_PUBLISH_SVR_H__
#include "IPC.h"
namespace LYW_CODE
{
    class PublishTaskBase::PublishSvr
    {
    private:
        IPC * m_IPC;
        PublishSvr();
    public:
        ~PublishSvr();

        static PublishTaskBase::PublishSvr * GetInstance();

        int Publish();
    };
}
#endif
