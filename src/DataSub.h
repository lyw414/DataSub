#include <stdio.h>

namespace LYW_CODE
{
    /**
     * @brief           IPC服务
     */
    class IPC 
    {
    private:
        IPC();
    public:
        static IPC * GetInstance();
    };


    class SubscribeSvr()
    {
    private :
        IPC * m_ipc;
        SubscribeSvr();
    public:
        ~SubscribeSvr();

    };

    class PublishSvr()
    {
    private :
        IPC * m_ipc;
        PublishSvr();
    public:
        ~PublishSvr();
    };


    class Subscribe
    {
    private:
        SubscribeSvr * m_sub;
    public:
        SubscribeBase();
        ~SubscribeBase();
    };

    class Publish
    {
    private:
        PublishSvr * m_pub;
    public:
        
    };

}
