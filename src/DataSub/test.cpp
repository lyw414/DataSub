#include "DataSub.hpp"

void HandleFunc(void * msg, int lenOfMsg, void * userParam)
{
    printf("XXXXXXXXXXXXX %p %d %p\n", msg, lenOfMsg, userParam);
}

int main()
{
    RM_CODE::DataSub dataSub;

    char buf[1024] = {0};

    dataSub.Init("Test", 1024, 1024, 4);

    int msgHandle = dataSub.RegisterMsg(11);
    
    dataSub.Subcribe(msgHandle, RM_CODE::Function3<void(void *, int, void *)> (HandleFunc), NULL);

    dataSub.Publish(msgHandle, buf, 1024);

    dataSub.UnRegisterMsg(msgHandle);

    return 0;
}
