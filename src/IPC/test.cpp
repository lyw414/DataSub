#include "Message_IPC.h"


int main()
{
    pid_t pid;
    
    int count = 5;

    while(true)
    {
        pid = fork();
        if (pid == 0)
        {
            break;
        }

        //father do 
        count--;
        if (count <= 0)
        {
            return 0;
        }
    }

    //child do
    LYW_CODE::Message_IPC * ipc = new LYW_CODE::Message_IPC();
    ipc->Connect();
    for (int iLoop = 0; iLoop < 10; iLoop++)
    {
        ipc->Lock();
        ipc->Test();
        ipc->Unlock();
    }
    return 0;
}
