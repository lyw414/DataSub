#include "ProTransAPI.h"

int main()
{
    LYW_CODE::ProTrans proTrans;

    proTrans.Destroy(0x02);

    proTrans.AddTransCFG(0,32,16);
    proTrans.AddTransCFG(1,16,16);
    proTrans.AddTransCFG(2,32,16);

    proTrans.Init(0x02);
    
    int t = 12345;

    int len = 0;
    LYW_CODE::ProTrans::IndexBlock_t BH;
    BH.index = -1;
    t = 1;
    proTrans.Write(0, (unsigned char *)&t, 4);
    t = 2;
    proTrans.Write(0, (unsigned char *)&t, 4);
    t = 3;
    proTrans.Write(0, (unsigned char *)&t, 4);
    t = 0;
    proTrans.Read(&BH, 0, (unsigned char *)&t, 4, &len);
    printf("%d\n", t);
    proTrans.Read(&BH, 0, (unsigned char *)&t, 4, &len);
    printf("%d\n", t);
    return 0;
}
