//#include "TaskPoolAPI.h"
#include "TaskPool.hpp"
#include <stdio.h>

class X {
public:
    void func(void * param, int len, void * userParam)
    {
        printf("len[%d]::%p\n", len, param);
    }

};

int main()
{
    //RM_CBB_TaskPoolInit(1024, 4);
    //RM_CBB_TaskPoolUnInit();
    X x;
    RM_CODE::TaskPool taskPool;

    xint32_t handle;

    RM_CODE::Function3<void(void *, xint32_t, void * )> cb(&X::func, &x);

    taskPool.Init(1024, 4);
    
    handle = taskPool.RegisterNormalTask(cb, NULL);

    printf("%d\n", handle);

    pause();

    return 0;
}

