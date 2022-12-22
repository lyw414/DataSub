#include "DataSubFunc.hpp"

#include <stdio.h>

int Show(int a, int b, int c)
{
    printf("ST %d %d %d\n", a, b, c);
    return 0;
}


class A
{
public:
    int Show(int a, int b, int c)
    {
        printf("OBJ %d %d %d\n", a, b, c);
        return 1;
    }
};

int main()
{
    A a;

    LYW_CODE::SubErrorCB<int(int,int,int)>  errCB_st(Show);

    LYW_CODE::SubErrorCB<int(int,int,int)>  errCB_ob(&A::Show, &a);


    errCB_st(1,2,3);
    errCB_ob(1,2,3);

    return 0;

}
