//#include "TaskPoolAPI.h"
#include "TaskPool.hpp"
#include <stdio.h>

class X {
public:
    void func(void * param, int len, void * userParam)
    {
        printf("len[%d]::%p %p\n", len, param, userParam);
    }

};

int main()
{
    //RM_CBB_TaskPoolInit(1024, 4);
    //RM_CBB_TaskPoolUnInit();
    X x;
    RM_CODE::TaskPool taskPool;

    void * handle;

    void * timeTask;

    RM_CODE::Function3<void(void *, xint32_t, void * )> cb(&X::func, &x);

    taskPool.Init(1024, 4);
    
    handle = taskPool.RegisterNormalTask(cb, (void *)0x23, NULL,  false);

    timeTask = taskPool.RegisterTimeTask(cb, (void *)0x22, 2000);
    
    taskPool.AsyncExecTask(handle, (void *)0x01, 0);
    taskPool.AsyncExecTask(handle, (void *)0x02, 0);
    taskPool.AsyncExecTask(handle, (void *)0x03, 0);
    taskPool.AsyncExecTask(handle, (void *)0x04, 0);
    taskPool.AsyncExecTask(handle, (void *)0x05, 0);
    taskPool.AsyncExecTask(handle, (void *)0x06, 0);
    taskPool.AsyncExecTask(handle, (void *)0x07, 0);
    taskPool.AsyncExecTask(handle, (void *)0x08, 0);
    taskPool.AsyncExecTask(handle, (void *)0x09, 0);
    taskPool.AsyncExecTask(handle, (void *)0x10, 0);
    taskPool.AsyncExecTask(handle, (void *)0x11, 0);

    pause();

    return 0;
}

