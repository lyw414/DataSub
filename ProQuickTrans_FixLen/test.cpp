#include <sys/time.h>
#include <stdlib.h>

#include "ProQuickTransAPI.h"

typedef struct _Data {
    int index;
    long long time;
    char tmp[256];
} Data_t;


void WrDo(int index, int count, int interval)
{
    //LYW_CODE::ProTrans proTrans;

    ProQuickTransHandle handle = RM_CBB_ProQuickTransInit(0x02);
    //proTrans.Init(0x02);
    Data_t data;

    struct timeval wr;
    struct timeval begin;
    struct timeval end;

    long long times = 0;
    long long c = count;

    data.index = index;

    int res = 0;

    for (int iLoop = 0; iLoop < count; iLoop++)
    {
        //printf("%d %d\n",index, iLoop);
        ::gettimeofday(&wr,NULL);
        data.time = wr.tv_sec * 1000000 + wr.tv_usec ;
        //data.time = iLoop;
        ::gettimeofday(&begin,NULL);
        //proTrans.Write(0, (unsigned char *)&data, sizeof(Data_t));

        res = RM_CBB_ProQuickTransWrite(handle, 0, (xbyte_t *)&data, sizeof(Data_t), 1000, 0);
        ::gettimeofday(&end,NULL);
        times += (end.tv_sec -  begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec);
        if (interval <= 0)
        {
            ::sched_yield();
        }
        else
        {
            ::usleep(interval);
        }
    }
    
    data.time = 0;
    RM_CBB_ProQuickTransWrite(handle, 0, (xbyte_t *)&data, sizeof(Data_t), 0, 0);
    printf("Index %d Wr TPS %lld\n", index, c * 1000 / times);

}

void RdDo(int index, int interval)
{

    ProQuickTransHandle handle = RM_CBB_ProQuickTransInit(0x02);

    ProQuickTransNodeIndex_t IN;
    IN.index = -1;
    IN.mode = 0;

    int t;
    int len;

    struct timeval rd;
    long long useTime = 0;

    long long max = 0;

    Data_t data;

    int res;

    int Num = 0;
    long long xx = 0;
    while(true)
    {
        res = RM_CBB_ProQuickTransRead(handle, &IN, 0, (xbyte_t *)&data, sizeof(Data_t), 1000, 0);
        ::gettimeofday(&rd,NULL);
        xx = rd.tv_sec * 1000000 + rd.tv_usec - data.time;
        //useTime = data.time;
        Num++;
        if (xx > 100000000)
        {
            printf("%dR %d::ReadCount %d maxInteval %lld interval %lld\n ",index, data.index, Num, max, useTime / Num);
            break;
        }
        else
        {
           useTime += xx;
           if (xx > max)
           {
                max = xx;
           }
        }
        //::usleep(interval);

        if (interval <= 0)
        {
            ::sched_yield();
        }
        else
        {
            ::usleep(interval);
        }

    }
}

int main(int argc, char * argv[])
{

    RM_CBB_ProQuickTransDestroy(0x02);

     

    int WNum = 1;
    int RNum = 10;

    int WCount = 1000000;
    int WInterval = 0;
    int RInterval = 0;


    if (argc > 1)
    {
        WNum = atoi(argv[1]);
    }

    if (argc > 2)
    {
        RNum = atoi(argv[2]);
    }

    if (argc > 3)
    {
        WInterval = atoi(argv[3]);
    }


    if (argc > 4)
    {
        RInterval = atoi(argv[4]);
    }


    if (argc > 5)
    {
        WCount = atoi(argv[5]);
    }

     
    for (int iLoop = 0; iLoop < RNum; iLoop++)
    {
        if (fork() == 0)
        {
            RdDo(iLoop, RInterval);
            exit(0);
        }
    }

    for (int iLoop = 0; iLoop < WNum; iLoop++)
    {
        if (fork() == 0)
        {
            WrDo(iLoop,WCount, WInterval);
            exit(0);
        }
    }
    
    while(true)
    {
        sleep(2);
    }
    return 0;
}
