#include "ProQuickTrans.hpp"


int main(int argc, char * argv[])
{
    RM_CODE::ProQuickTrans proTrans;
    RM_CODE::ProQuickTransCFG_t cfg[2];

    cfg[0].id = 0;
    cfg[0].size = 4096;

    cfg[1].id = 1;
    cfg[1].size = 4096;

    proTrans.Destroy(0x02);
    proTrans.Init(0x02,cfg,2);


    return 0;
}
