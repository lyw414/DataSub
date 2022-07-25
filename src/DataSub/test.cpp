//#include "ProSubTable.hpp"
#include "SimpleFunc.hpp"

#include <string>
#include <stdio.h>

//int fork_do(int id)
//{
//    LYW_CODE::ProSubTable proSubInfo(0x2A);
//    proSubInfo.Connect();
//    for (int iLoop = id * 32;iLoop < (id + 1) * 32; iLoop++)
//    {
//        proSubInfo.ProRegister(iLoop);
//    }
//    proSubInfo.DisConnect();
//    return 0;
//}

void Show(int a, double b, std::string & c)
{
    printf("XXXX\n");
}


class X {

public:
    void Show(int a, double b, std::string & c)
    {
        printf("XXXX\n");
    }
};

int main()
{
    X ss; 

    std::string p = "222";

    LYW_CODE::DataSubHandleFunc<void(int, double, std::string &)> xx(&X::Show, &ss);

    xx(1, 1.2, p);



    //int index = 0; 
    //int ret = 0;
    //LYW_CODE::ProSubTable proSubInfo(0x2A);
    //proSubInfo.Destroy();
    //proSubInfo.Create();
    //proSubInfo.Connect();

    //int proIndex1 = proSubInfo.ProRegister(12);
    //int proIndex2 = proSubInfo.ProRegister(13);
    //
    //int msgIndex1 = 0;
    //int msgIndex2 = 0;


    //unsigned char subMap[4] = {0};

    //int len = 0;
    //

    //for (int iLoop = 0; iLoop < 64; iLoop++)
    //{
    //    msgIndex1 = proSubInfo.MsgRegister(iLoop, proIndex1, 12);
    //    msgIndex2 = proSubInfo.MsgRegister(iLoop, proIndex2, 13);
    //    printf("%d %d\n", msgIndex1, msgIndex2);
    //}

    //proSubInfo.printfMsg();


    //proSubInfo.QuerySubPro(33, 33, subMap, 4, len);

    //printf("len::%d subMap %02X %02X %02X %02X\n",len, subMap[0], subMap[1], subMap[2], subMap[3]);
    //
    //
    //proSubInfo.DisConnect();
    return 0;

}
