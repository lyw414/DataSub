//#include "TaskPoolAPI.h"
#include <stdio.h>
#include "TaskPoolAPI.h"

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

class X {
public:
    void func(void * param, int len, void * userParam)
    {
        unsigned int x = (unsigned long)param;

        if ((x % 512) == 0)
        {
            printf("len[%d]::user[%p] [%p]\n", len, userParam, param);
        }

    }

    void error(void * param, int len, void * userParam)
    {
        printf("Timeout len[%d]::user[%p] [%p]\n", len, userParam, param);
    }



};



void * thread_do(void * ptr)    
{
    X x;
    int index = 0;
    TaskPoolCB_t cb(&X::func, &x);
    
    //RM_CODE::Function3<void(void *, xint32_t, void * )> er(&X::error, &x);
    void * handle = RM_CBB_RegisterNormalTask(cb, (void *)ptr, false, 0);

    while(true)
    {
        index++;
        if (RM_CBB_AddTask(handle, (void *)index, 0) < 0)
        {
            //printf("End\n");
            break;
        }
        usleep(10000);
    }
}

int main()
{
    //RM_CBB_TaskPoolInit(1024, 4);
    //RM_CBB_TaskPoolUnInit();
    X x;

    void * handle;

    void * timeTask;

    RM_CBB_TaskPoolCreate(1024, 4);
    
    TaskPoolCB_t cb(&X::func, &x);
    timeTask = RM_CBB_RegisterTimeTask(cb, (void *)0x22, 2000);

    pthread_t pthread;

    for(int iLoop = 0; iLoop < 32; iLoop++)
    {
        pthread_create(&pthread, NULL, thread_do, (void *)(iLoop + 1));
    }


    sleep(5);

    RM_CBB_TaskPoolDestroy();

    pause();

    return 0;
}

