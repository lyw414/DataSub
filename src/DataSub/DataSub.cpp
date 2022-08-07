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
        if (m_name != "")
        {
            return -1;
        }

        if (ProClientTable::GetInstance()->Register(name, m_key) >= 0)
        {
            m_name = std::string(name);
            return 0;
        }
        else
        {
            m_name = "";
            return -1;
        }
    }

    int DataSub::UnInit()
    {
        return 0;
    }

}
