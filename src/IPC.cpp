#include "IPC.h"
namespace LYW_CODE
{
    IPC::IPC()
    {

    }

    IPC::~IPC()
    {

    }

    IPC * IPC::GetInstance()
    {
        static IPC * instance = new IPC();
        return instance;
    }
}
