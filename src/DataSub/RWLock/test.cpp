#include "RWLock.hpp"
#include "sys/time.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int tag;
int * RdRecord;
unsigned long long RdCount;

pthread_mutex_t lock;
int data;
int data1;

int RInterval;
int WDelay;
int WInterval;

time_t RWTime;
time_t sysTime;

int useTag;
int endCount;

LYW_CODE::RWDelaySafeLock * m_lock;

void * Wr(void * param)
{

    if (useTag == 0)
    {
        while(endCount > 0)    
        {
            data1 = data;
            pthread_mutex_lock(&lock);
            data = 1000000;
            usleep(1000);
            data = data1 + 1;
            pthread_mutex_unlock(&lock);
            ::usleep(WInterval);
        }
    }
    else
    {

        while(endCount > 0)    
        {

            data1 = data;
            m_lock->WriteLock(WDelay);
            data = 1000000;
            usleep(100);
            data = data1 + 1;
            m_lock->WriteUnLock(WDelay);
            data1 = 0;
            ::usleep(WInterval);
        }
    }

    return NULL;
}

void * Rd(void * param)
{
    int * index = (int *)param;
    int lastData = 0;
    int tmp = 0;
    unsigned long long loop = 0;
    int st = 0;
    struct timeval begin, end;
    
    unsigned long t;


    if (useTag == 0)
    {
        gettimeofday(&begin,NULL);

        while(loop < RdCount)
        {
            loop++;
            pthread_mutex_lock(&lock);
            tmp = data;
            pthread_mutex_unlock(&lock);

            if ((lastData != tmp) && (lastData > tmp)) 
            {
                printf("Error L[%d] C[%d]\n", lastData, tmp);
            }
            else
            {
                usleep(RInterval);
            }

            lastData = tmp;

        }
        gettimeofday(&end,NULL);
        t = (end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec);
        printf("Sys Read %d %ld\n",*index,t);
        pthread_mutex_lock(&lock);
        sysTime += t;
        endCount--;
        pthread_mutex_unlock(&lock);
    }
    else
    {
        gettimeofday(&begin,NULL);
        while(loop < RdCount)
        {

            loop++;
            st = m_lock->ReadLock();
            if (st == 0)
            {
                tmp = data;
            }
            else
            {
                tmp = data1;
            }

            m_lock->ReadUnLock(st);

            if ((lastData!= tmp) && (lastData > tmp)) 
            {
                printf("Error L[%d] C[%d]\n", lastData, tmp);
            }
            else
            {
                //usleep(RInterval);
            }

            lastData = tmp;

        }
        gettimeofday(&end,NULL);
        t = (end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec);

        printf("RW Read %d %ld\n",*index,t);
        pthread_mutex_lock(&lock);
        RWTime += t;
        endCount--;
        pthread_mutex_unlock(&lock);
    }

    return NULL;

}


int main(int argc, char * argv[])
{
    pthread_mutex_init(&lock,NULL);
    LYW_CODE::RWDelaySafeLock lock_;
    m_lock = &lock_;
    data = 0;

    int threadNum = 1;
    int threadNum1 = 1;


    if (argc >= 2)
    {
        useTag = atoi(argv[1]);
    }

    if (argc >= 3)
    {
        endCount = threadNum = atoi(argv[2]);
        RdRecord = (int *)::malloc( threadNum  * sizeof(int));
    }

    if (argc >= 4)
    {
       threadNum1 = atoi(argv[3]);
    }

    if (argc >= 5)
    {
       RdCount = atoi(argv[4]);
    }

    if (argc >= 6)
    {
       RInterval = atoi(argv[5]);
    }

    if (argc >= 7)
    {
       WDelay = atoi(argv[6]);
    }

    if (argc >= 8)
    {
       WInterval = atoi(argv[7]);
    }

    pthread_t * h = (pthread_t *)::malloc(sizeof(pthread_t) *threadNum);
    pthread_t * h1 = (pthread_t *)::malloc(sizeof(pthread_t) * threadNum1);

    for (int iLoop = 0; iLoop < threadNum1; iLoop++)
    {
        pthread_create(h1 +iLoop, NULL, Wr, NULL);
    }

    for (int iLoop = 0; iLoop < threadNum; iLoop++)
    {
        pthread_create(h +iLoop, NULL, Rd, &(RdRecord[iLoop]));
    }

    for (int iLoop = 0; iLoop < threadNum; iLoop++)
    {
        pthread_join(h[iLoop], NULL);
    }
    for (int iLoop = 0; iLoop < threadNum1; iLoop++)
    {
        pthread_join(h1[iLoop], NULL);
    }
 
    printf("sys %ld  RW %ld\n", sysTime, RWTime);

    return 0;
}
