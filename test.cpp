#include "DataSub.h"


int main()
{
    LYW_CODE::SubscribeTaskBase test;

    LYW_CODE::PublishTaskBase pub;

    test.registerSubTask(2);

    pub.Publish(1,std::string("2222"));
    
    return 0;
}
