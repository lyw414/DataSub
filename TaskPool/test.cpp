//#include "TaskPoolAPI.h"
#include <stdio.h>
#include "TaskPoolAPI.h"

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int * recordArray;

class X {
public:
    void func(void * param, int len, void * userParam)
    {
        long index = (long)userParam;
        __sync_add_and_fetch(&recordArray[index * 2],1);
        //unsigned int x = (unsigned long)param;

        //if ((x % 512) == 0)
        //{
        //    //printf("len[%d]::user[%p] [%p]\n", len, userParam, param);
        //}

    }

    void error(TaskPoolErrCode_e errCode, void * param, int len, void * userParam)
    {
        long index = (long)userParam;
        __sync_add_and_fetch(&recordArray[index * 2 + 1],1);
        //printf("errCode[%d] len[%d]::user[%p] [%p]\n",errCode, len, userParam, param);
    }

};



void * thread_do(void * ptr)    
{
    X x;

    int index = 0;
    TaskPoolCB_t cb(&X::func, &x);
    TaskPoolErrCB_t errCB(&X::error, &x);
    //RM_CODE::Function3<void(void *, xint32_t, void * )> er(&X::error, &x);
    void * handle = RM_CBB_RegisterNormalTask(cb, errCB, (void *)ptr, true, 20);

    while(index < 1000000)
    {
        index++;
        if (RM_CBB_AddTask(handle, (void *)index, 0) < 0)
        {
            printf("Error\n");
            break;
        }
        usleep(0);
    }
}

int main()
{
    //RM_CBB_TaskPoolInit(1024, 4);
    //RM_CBB_TaskPoolUnInit();
    X x;

    char buf[2048] = {0};
    void * handle;

    void * timeTask;
    void * timeTask1;

    RM_CBB_TaskPoolCreate(1024, 4);
    
    TaskPoolCB_t cb(&X::func, &x);
    TaskPoolErrCB_t errCB(&X::error, &x);
    timeTask = RM_CBB_RegisterTimeTask(cb, errCB, (void *)0x00, 4000, false);
    timeTask1 = RM_CBB_RegisterTimeTask(cb, errCB, (void *)0x01, 0, false);

    pthread_t pthread;

    recordArray = (int *)malloc(sizeof(int) * 32 * 2);

    memset(recordArray, 0x00, sizeof(int) * 32 * 2);

    //for(int iLoop = 0; iLoop < 32; iLoop++)
    //{
    //    pthread_create(&pthread, NULL, thread_do, (void *)(iLoop));
    //}

    while(true)
    {
        sleep(2);
        memset(buf, 0x00, 2048);
        printf("*****************************************\n");
        for (int iLoop = 0; iLoop < 2; iLoop++)
        {
            printf("index %d :: success[%d] failed [%d] total[%d]\n", iLoop, recordArray[iLoop * 2], recordArray[iLoop * 2 + 1],recordArray[iLoop * 2] + recordArray[iLoop * 2 + 1]);
        }

        if (timeTask1 != NULL)
        {
            RM_CBB_UnRegisterTask(timeTask1);
            timeTask1 = NULL;
        }

    }
    //sleep(5);
    pause();

    RM_CBB_TaskPoolDestroy();

    pause();

    return 0;
}

