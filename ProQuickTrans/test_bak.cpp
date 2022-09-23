#include "ProQuickTransAPI.h"

void WrDo()
{
    PrOQuickTransHandle handle = RM_CBB_ProQuickTransInit(0x02);

    int res = 0;

    for (int iLoop = 0; iLoop < 10000; iLoop++)
    {
        RM_CBB_ProQuickTransWrite(handle, 0, (xbyte_t *)&iLoop, 4, 1000);

        sleep(2);
    }

}


void RdDo()
{
    ProQuickTransNodeIndex_t IN;
    IN.index = -1;
    IN.mode = 0;
    PrOQuickTransHandle handle = RM_CBB_ProQuickTransInit(0x02);
    int out;
    int res;
    while(true)
    {
        res = RM_CBB_ProQuickTransRead(handle, &IN, 0, (xbyte_t *)&out, 4, 0);
        printf("*************** res %d %d\n", res, out);
    }
}



int main()
{
    int Num = 0;
    int out;
    int RNum = 5;
    RM_CBB_ProQuickTransDestroy(0x02);

    if (fork() == 0)
    {
        RdDo();
        exit(0);
    }
    
    for (int iLoop = 0; iLoop < RNum; iLoop++)
    {
        if (fork() == 0)
        {
            WrDo();
            exit(0);
        }
    }

    while(true)
    {
        sleep(10);
    }
    return 0;
}
