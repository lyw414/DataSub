#include "ProQuickTrans.hpp"
#include <time.h>

void DoInit()
{
    RM_CODE::ProQuickTrans proTrans;
    RM_CODE::ProQuickTransCFG_t cfg[2];

    cfg[0].id = 0;
    cfg[0].size = 8192;

    proTrans.Destroy(0x02);
    proTrans.Init(0x02,cfg,1);

}

void DoWrite()
{
    RM_CODE::ProQuickTrans proTrans;
    RM_CODE::ProQuickTransCFG_t cfg[2];

    time_t now;
    int exterLen = 0;

    char data[256] = {0};
    time(&now);
    srand(now);
    proTrans.Init(0x02, NULL, 0);
    
    for (int iLoop = 0; iLoop < 10000000; iLoop++)
    {
        exterLen = rand();
        exterLen = exterLen % 128 + 4;
        *(int *)data = iLoop;
        proTrans.Write(0,data, exterLen, 0 , 1);
        printf("%d\n", iLoop);
        usleep(10000);
    }
}

void DoRead()
{
    RM_CODE::ProQuickTrans proTrans;
    ProQuickTransReadIndex_t readIndex;

    char outData[256] = {0};
    int outLen = 0;
    int lastIndex = -1;
    int index = 0;


    proTrans.Init(0x02, NULL, 0);
    while(true)
    {
        proTrans.Read(0, &readIndex, outData, 256, &outLen, PRO_TRANS_READ_ORDER, 0, 1);
        index = *(int *)outData;

        printf("%d\n", index);

        //if (index <= lastIndex)
        //{
        //    printf("Error\n");
        //}
        //else
        //{
        //    lastIndex = index;
        //}
    }

}


int main(int argc, char * argv[])
{

    int mode = 0;

    char outData[16] = {0};
    int outLen = 0;


    if (argc > 1) 
    {
        mode = atoi(argv[1]);
    }

    switch(mode)
    {
        case 0:
            //DoInit();
            DoWrite();
            break;
        case 1:
            //DoInit();
            DoRead();
            break;
        case 2:
            DoInit();
            break;
    }

    return 0;
}
