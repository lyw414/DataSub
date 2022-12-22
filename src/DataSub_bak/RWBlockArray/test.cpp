#include "RWBlockArray_IPC.hpp"
#include <sys/time.h>
char gbuf[128];

int gSub;
int gPub;
int gCount;

class X
{
public:
    bool ReadFinish(void * data)
    {
        unsigned char * d = (unsigned char * )data;
        if ((::memcmp(data, gbuf, gSub)) == 0)
        {
            return true;
        }
        else
        {
            return false;
        }

        return true;
    }

    bool IsNeed(void * data, unsigned int size, void * param)
    {
        int index = *((int *)param);
        if (((char *)data)[index] == 0x01)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

};


void fork_sd()
{
    X x;
    int ret = 0;
    char buf[996];

    memset(buf, 0x00, 996);
    memset(buf, 0x01, gSub);

    LYW_CODE::RWBlockArray_IPC rwBlock(0x1A);

    //rwBlock.Connect(std::bind(&X::ReadFinish, &x, std::placeholders::_1));
    rwBlock.Connect(LYW_CODE::Function1<bool(void *)>(&X::ReadFinish, &x));


    int iLoop = 0;
    int record = 0;

    while(iLoop < gCount)
    {
        sprintf(buf + gSub, "%04d", iLoop);

        if (rwBlock.Write(buf, 900, 100) < 0)
        {
            //printf("full..............\n");
            record++;
        }
        else
        {
            iLoop++;
        }

        sleep(2);
    }

    struct timeval t;
    ::gettimeofday(&t, NULL);
    printf("E %ld.%ld wait:%d\n", t.tv_sec, t.tv_usec, record);
    return;
}

void fork_do(int id)
{
    X x;
    int myID =  id;
    int number = 0;
    LYW_CODE::RWBlockArray_IPC rwBlock(0x1A);
    //rwBlock.Connect(std::bind(&X::ReadFinish, &x, std::placeholders::_1));
    rwBlock.Connect(LYW_CODE::Function1<bool(void *)>(&X::ReadFinish, &x));
    char * data = NULL;
    LYW_CODE:: RWBlockArray_IPC::BlockHandle_t BH_t;
    LYW_CODE:: RWBlockArray_IPC::BlockHandle_t * BH = &BH_t;
    BH->index = 0;
    BH->id = 0;

    //BH = NULL;
    while(true)
    {
        //data = (char *)rwBlock.Read(BH, BH,std::bind(&X::IsNeed, &x, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), &myID);

        data = (char *)rwBlock.Read(BH, BH,LYW_CODE::Function3<bool(void *, unsigned int, void *) >(&X::IsNeed, &x), &myID);
        if (data != NULL)
        {
            printf("id %d :: %s\n", id, (char *)(data + gSub));
            ::memset(data + gSub + 1, 0x00, 700);
            data[myID] = 0x00;
            number++;
        }
        else
        {
            printf("id %d :: NILL\n",id);
        }

        if (number >= gCount * gPub)
        {
            printf("id %d :: recv count[%d]\n",id, number);
            number = 0;
        }
    }
}


int main()
{
    X x;
    int ret = 0;
    int iLoop = 0;

    memset(gbuf,0x00,128);
    //LYW_CODE::RWBlockArray_IPC::BlockHandle_t BH;
    //BH.index = 0;
    //BH.id = 0;

    gSub = 1;
    gPub = 1;
    gCount = 10000;

    LYW_CODE::RWBlockArray_IPC rwBlock(0x1A);

    rwBlock.Destroy();

    rwBlock.Create(1024, 1024);

    //time_t t1, t2;
    //::time(&t1);

    struct timeval t;
    ::gettimeofday(&t, NULL);
    printf("B %ld.%ld\n", t.tv_sec, t.tv_usec);

    for (int loop = 0; loop < gSub; loop++) 
    {
        if (fork() == 0)
        {
            //child do
            fork_do(loop);
            printf("End\n");
            return 0;
        }
    }

    for (int loop = 0; loop < gPub; loop++) 
    {
        if (fork() == 0)
        {
            //child do
            fork_sd();
            return 0;
        }
    }


    while(true)
    {
        sleep(100);
    }
    return 0;
}
