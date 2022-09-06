//#include "ProSubTable.hpp"
#include "DataSub.h"


class X 
{
public:
    void Do(void * msg, unsigned int len, void * userParam)
    {
        printf("param %p len %d subcribe %s\n",userParam, len, (char *)msg);
    }
};

int main()
{
    X x;
    
    char buf[] = "12345678";
    LYW_CODE::DataSub dataSub;
    dataSub.Init("Test");


    LYW_CODE::DataSub dataSub1;
    dataSub1.Init("Test");

    dataSub.Subcribe(11, LYW_CODE::Function3 <void(void *, unsigned int, void *)> (&X::Do, &x), NULL, 0, 0);
    while(true)
    {
        dataSub.Publish(11, buf, 9);
        sleep(2);
    }
    return 0;
}
    
