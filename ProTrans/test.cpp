#include "ProQuickTrans.hpp"
#include <sys/time.h>
#include <stdlib.h>

void DoSleep(int interval)
{
    if (interval == 0)
    {
        ::sched_yield();
    }
    else
    {
        ::usleep(interval * 1000);
    }
}

void DoWrite(int count,int interval, int spinInterval)
{
    time_t sid;
    time(&sid);
    srand(sid);
    int exLen = 0;

    RM_CODE::ProQuickTrans proTrans;
    char buf[4096] = {0};
    int * num = (int *)buf;
    proTrans.Init(0x02, NULL, 0);
    for ((*num) = 0; (*num) < count; (*num)++)
    {
        exLen = rand() ;
        exLen = exLen % 1024 + 4;
        //exLen = 1024;
        proTrans.Write(0, buf, exLen, 0, spinInterval);
        printf("Send %d %d\n",*num, exLen);
        DoSleep(interval);
    }
}


void DoRead(int interval, int spinInterval)
{
    RM_CODE::ProQuickTrans proTrans;
    RM_CODE::ProQuickTransReadRecord_t readRecord;
    char buf[4096] = {0};
    int res = 0;
    int outLen;
    proTrans.Init(0x02, NULL, 0);
    while(true)
    {
        res = proTrans.Read(0, &readRecord, buf, 1024 + 4, &outLen, RM_CODE::PRO_READ_ORDER, 0, spinInterval);
        printf("res [%d] buf [%d]\n", res, *((int *)buf));
        DoSleep(interval);
    }
}


int main(int argc, char * argv[])
{
    char buf[1024] = {0};
    char rBuf[1024] = {0};
    int outLen = 0;
    int res = 0;

    int mode = 2;


    memset(buf, 0x31, sizeof(buf));
    buf[1023]  = '\0';


    if (argc > 1) 
    {
        mode = atoi(argv[1]);
    }
    
    

    if (mode == 0)
    {
        DoWrite(1000000, 1000, 100);
    }
    else if (mode == 1)
    {
        DoRead(0, 100);
    }
    else
    {
        RM_CODE::ProQuickTrans proTrans;
        RM_CODE::ProQuickTransCFG_t cfg[2];

        cfg[0].id = 0;
        cfg[0].size = 8192;

        cfg[1].id = 1;
        cfg[1].size = 1024;

        proTrans.Destroy(0x02);
        proTrans.Init(0x02,cfg,2);

        //DoRead(0, 1);
        //DoWrite(100000, 100, 1);
    }

    return 0;

}
