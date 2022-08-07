#include "DataSub.h"

#include "ProClientTable.hpp"

namespace LYW_CODE
{
    DataSub::DataSub()
    {
        m_name = "";

        m_key = 0;
    }

    int DataSub::Init(const char * name)
    {
        int res = 0;

        if (m_name != "")
        {
            return -1;
        }

        if ((res = ProClientTable::GetInstance()->Register(name, m_key)) >= 0)
        {
            m_name = std::string(name);
            return 0;
        }
        else
        {
            m_name = "";
            return -1;
        }
        
        if (res == 0)
        {
            //需要拉起服务


        }

    }

    int DataSub::UnInit()
    {
        //最后一个注销的线程 需要终止服务
        return 0;
    }

}
