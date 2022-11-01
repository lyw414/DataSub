#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include <sys/types.h>          
#include <sys/socket.h>
#include <arpa/inet.h>


int pInterval;

typedef struct _Data {
    int index;
    long long time;
    char tmp[256];
} Data_t;


long long g_count;

void * ThreadDo(void * ptr)
{
    
    int sck = *(int *)ptr;

    struct timeval wr;

    struct timeval begin;

    struct timeval end;


    long long times = 0;

    long long c = g_count;

    Data_t data;

    int Num = 0;

    int res = 0;

    
    data.index = sck;

    for (int iLoop = 0; iLoop < g_count; iLoop++)
    {
        ::gettimeofday(&wr,NULL);
        data.time = wr.tv_sec * 1000000 + wr.tv_usec ;
        ::gettimeofday(&begin,NULL);
        res = send(sck, &data, sizeof(Data_t), 0);
        ::gettimeofday(&end,NULL);
        times += (end.tv_sec -  begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec);

        if (pInterval <= 0)
        {
            ::sched_yield();
        }
        else
        {
            ::usleep(pInterval);
        }
    }

    data.time = 0;
    send(sck, &data, sizeof(Data_t), 0);

    printf("Index %d Wr TPS %lld wwr %d www %d\n", sck, c * 1000 / times, 0, 0);
    sleep(10);
    return NULL;
}

void WrDo(int index, int cliCount, int count, int interval)
{
    int sck = 0;

    struct timeval wr;

    struct timeval begin;

    struct timeval end;


    long long times = 0;

    long long c = count;

    g_count = count;

    Data_t data;

    int Num = 0;

    int * cli = (int *)::malloc(sizeof(int) * cliCount);

    struct sockaddr_in server, client;
    sck = socket(AF_INET,SOCK_STREAM,0);

    server.sin_family = AF_INET;
    server.sin_port = htons(9999);
    server.sin_addr.s_addr = INADDR_ANY;
    int sockaddr_size = sizeof(struct sockaddr);

    int option = 1;

    int res = 0;

    setsockopt (sck, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    bind(sck, (struct sockaddr* )&server, sizeof(server));

    listen(sck,1024);

    pthread_t handle[64];

    while(true) 
    {
        cli[Num] = accept(sck ,(struct sockaddr *)&client,(socklen_t * )&sockaddr_size);
        if (cli[Num] > 0)
        {
            pthread_create(&handle[Num],NULL, ThreadDo, &cli[Num]);
            Num++;
        }

        if(Num >= cliCount)
        {
            break;
        }
    }
    
    sleep(10000);
    close(sck);
    return;
}

void RdDo(int index, int interval)
{
    int t;
    struct timeval rd;
    long long useTime = 0;

    long long max = 0;

    Data_t data;

    unsigned char * ptr = (unsigned char *)&data;
    
    int len = 0;

    int res = 0;
    int Num = 0;
    long long xx = 0;

    int sck = 0;

    struct sockaddr_in client;

    sck = socket(AF_INET,SOCK_STREAM,0);

    client.sin_family = AF_INET;
    client.sin_port = htons(9999);
    client.sin_addr.s_addr = INADDR_ANY;

    if (connect(sck, (struct sockaddr* )&client, sizeof(client)) < 0)
    {
        printf("connect Error!\n");
        return;
    }

    while(true)
    {
        len = 0; 
        while(len < sizeof(Data_t))
        {
            res = recv(sck, ptr + len, sizeof(Data_t) - len, 0);
            if (res > 0)
            {
                len += res;
            }
            else
            {
                usleep(0);
            }
        }
        ::gettimeofday(&rd,NULL);
        xx = rd.tv_sec * 1000000 + rd.tv_usec - data.time;
        Num++;
        if (xx > 1000000)
        {
            printf("%dR %d::ReadCount %d maxInteval %lld interval %lld\n ",index, data.index, Num, max, useTime / Num);
            break;
        }
        else
        {
           useTime += xx;
           if (xx > max)
           {
                max = xx;
           }
        }
    }

    sleep(10);
}

int main(int argc, char * argv[])
{

    int WNum = 1;
    int RNum = 10;

    int WCount = 1000000;
    int WInterval = 0;
    int RInterval = 0;


    if (argc > 1)
    {
        WNum = atoi(argv[1]);
    }

    if (argc > 2)
    {
        RNum = atoi(argv[2]);
    }

    if (argc > 3)
    {
       pInterval = WInterval = atoi(argv[3]);
    }


    if (argc > 4)
    {
        RInterval = atoi(argv[4]);
    }


    if (argc > 5)
    {
        g_count = WCount = atoi(argv[5]);
    }

    for (int iLoop = 0; iLoop < WNum; iLoop++)
    {
        if (fork() == 0)
        {
            WrDo(iLoop, RNum, WCount,WInterval);
            exit(0);
        }
    }

    sleep(1);

    for (int iLoop = 0; iLoop < RNum; iLoop++)
    {
        if (fork() == 0)
        {
            RdDo(iLoop, RInterval);
            exit(0);
        }
    }


    
    while(true)
    {
        sleep(2);
    }
    return 0;
}
