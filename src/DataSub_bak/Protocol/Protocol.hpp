#ifndef  __LYW_CODE_PROTOCOL_H_FILE__
#define __LYW_CODE_PROTOCOL_H_FILE__

namespace LYW_CODE
{
namespace DataSub
{
    class Protocol
    {
    public:
#pragma(1)
    typedef struct _PublishInfo
    {
        int ID;
    } PublishInfo_t;


    /**
     * @brief       发布者
     *
     */
    typedef struct _Publiser
    {
        unsigned int proID; //进程发布id
        unsigned int pthID; //线程发布id
    } Publiser_t;
    
    /**
     * @brief       协议说明 ProtocolHead_t + ProSubInfo_t(变长) + Data_t(变长)
     *
     */
    typedef struct _ProtocolHead
    {
        unsigned int totalLen;          //报文总长
        unsigned int msgID;             //消息id
        Publiser_t publisher;           //发布者
        unsigned int response;          //是否为响应消息
        unsigned int timeout;           //ms -- 由守护进程进程超时管理
        unsigned int packageCount;      //拆包数
        unsigned int packageIndex;      //当前包index --- 当前包顺序号
        unsigned int dataLen;           //数据总长 --- 拼包时使用
        unsigned int dataIndex;         //数据开始索引 --- 拼包时使用
    } ProtocolHead_t;
    
    /**
     * @brief       订阅者信息
     *
     */
    typedef struct _MSGSubInfo          //订阅信息 
    {
        unsigned int len;               //字节数据
        unsigned char bitMap[0];        //订阅信息bitMap  订阅者id 为 bitMap 索引 0 未订阅 1 订阅
    } MSGSubInfo_t;
    
    /**
     * @brief       数据块
     *
     */
    typedef struct _Data
    {
        unsigned int len;               //data长度
        unsigned char data[0];          //数据
    } Data_t;
#pragma()

    private:
        ProtocolHead_t * m_head;
        MSGSubInfo_t * m_sub;
        Data_t * m_data;

    public:
        Protocol()
        {
            m_head = NULL;
            m_sub = NULL;
            m_data = NULL;
        }

        int Analysis(void * msg, int lenOfMsg)
        {
            unsigned int leftLen = lenOfMsg;

            unsigned char ptr = (unsigned char *)msg;

            m_head = NULL;
            m_sub = NULL;
            m_data = NULL;

            leftLen -= (sizeof(ProtocolHead_t) + sizeof(MSGSubInfo_t));
            if (msg == NULL || leftLen <= 0)
            {
                m_head = NULL;
                m_sub = NULL;
                m_data = NULL;
                return -1;
            }

            m_head = (ProtocolHead_t *)msg
            
            if (m_head->totalLen != lenOfMsg)
            {
                m_head = NULL;
                m_sub = NULL;
                m_data = NULL;
                return -1;
            }

            ptr += sizeof(ProtocolHead_t);

            m_sub = (MSGSubInfo_t *)(ptr);

            leftLen -= m_sub->len;

            if (leftLen < sizeof(Data_t))
            {
                m_head = NULL;
                m_sub = NULL;
                m_data = NULL;
                return -1;
            }
            
            ptr += sizeof(MSGSubInfo_t) + m_sub->len;

            m_data = (Data_t *)ptr;

            leftLen -= (sizeof(Data_t) + m_data->len);
            
            if (leftLen != 0)
            {
                m_head = NULL;
                m_sub = NULL;
                m_data = NULL;
                return -1;
            }

            return 0;
        }

        LYW_CODE::DataSub::Protocol::ProtocolHead_t * Head()
        {
            return m_head;
        }

        LYW_CODE::DataSub::Protocol::MSGSubInfo_t * Sub()
        {
            return m_sub;
        }

        LYW_CODE::DataSub::Protocol::Data_t * Data()
        {
            return m_data;
        }
    };
}
}
#endif
