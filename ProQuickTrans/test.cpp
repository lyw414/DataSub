#include "ProQuickTransAPI.h"
#include <stdlib.h>
#include <time.h>

void DoInit()
{
    //RM_CODE::ProQuickTrans proTrans;
    //RM_CODE::ProQuickTransCFG_t cfg[2];
    ProQuickTransHandle handle = NULL;
    RM_CBB_ProQuickTransDestroy(0x02);
    handle = RM_CBB_ProQuickTransInit(0x02);
    RM_CBB_ProQuickTransUnInit(&handle);
    return;
}

void DoWrite()
{

    ProQuickTransHandle handle = NULL;
    handle = RM_CBB_ProQuickTransInit(0x02);


    time_t now;
    int exterLen = 0;

    char data[256] = {0};
    time(&now);
    srand(now);
    
    for (int iLoop = 0; iLoop < 10000000; iLoop++)
    {
        exterLen = rand();
        exterLen = exterLen % 128 + 4;
        *(int *)data = iLoop;
        RM_CBB_ProQuickTransWrite(handle, 0, data, exterLen, 0 , 1);
        printf("%d\n", iLoop);
        usleep(10000);
    }
}

void DoRead()
{
    ProQuickTransReadIndex_t readIndex;
    ProQuickTransHandle handle = NULL;
    handle = RM_CBB_ProQuickTransInit(0x02);

    char outData[256] = {0};
    int outLen = 0;
    int lastIndex = -1;
    int index = 0;


    while(true)
    {
        RM_CBB_ProQuickTransRead(handle, 0, &readIndex, outData, 256, &outLen, PRO_TRANS_READ_ORDER, 0, 1);
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
