#include "TaskPool.hpp"
#include <stdio.h>
#include <string>


int testFixLenList()
{
    RM_CODE::FixLenList<int> x(8);

    RM_CODE::FixLenList<int>::ListNode_t * pNode = NULL;

    RM_CODE::FixLenList<int>::iterator it;

    x.Init();

    x.Push_back(0);
    x.Push_back(1);
    x.Push_back(2);
    pNode = x.Push_back(3);
    x.Push_back(4);
    x.Push_back(5);
    x.Push_back(6);
    x.Push_back(7);

    pNode = x.Erase(pNode);
    x.Pop_back();
    x.Pop_back();
    x.Pop_front();

    x.Push_back(6);
    x.Push_back(7);
    //x.Push_back(8);
    it = x.Begin();
    it = x.Erase(it);

    x.Push_back(8);
    x.Push_back(9);
    x.Push_back(10);

    for(;it != x.End(); it++)
    {
        if (*it == 1)
        {
            it = x.Erase(it);
        }
        printf("%d\n", *it);
    }

    x.UnInit();
    return 0;
}

class XX
{
public:
    void doTask(void * param, int len, void * userParam)
    {
        XX * self = (XX *)userParam;
        self->userDo();
        char * data = (char *)param;
        printf("len[%d] recv %s\n", len, (char *)param);
        return;
    }

    void doTask1(void * param, int len, void * userParam)
    {
        printf("Task1 %p .............\n", userParam);
        return;
    }

    void userDo( )
    {
        //printf("user do \n");
    }

};

int main()
{
    int count = 0;
    XX x;
    RM_CODE::TaskPool taskPool(1024, 4);
    RM_CODE::TaskPool::CallBack_t cb(&XX::doTask, &x);
    RM_CODE::TaskPool::CallBack_t cb1(&XX::doTask1, &x);
    RM_CODE::TaskPool::Task_t * task;
    RM_CODE::TaskPool::Task_t * task1;
    RM_CODE::TaskPool::TaskAttr_t taskAttr; 

    char buf[32] = {0};

    memset(buf, 0x31, 31);

    buf[31] = 0x00;

    taskPool.Init();

    taskPool.SetNormalTaskAttr(&taskAttr, true, 5, 1000);
    task = taskPool.RegisterTask(cb, &x, &taskAttr);


    taskPool.SetTimeTaskAttr(&taskAttr, 1000, true, 5, 1000);
    task1 = taskPool.RegisterTask(cb1, &x, &taskAttr);

    buf[0] = '1';
    buf[1] = 0x00;
    buf[2] = '2';
    buf[3] = 0x00;
    buf[4] = '3';
    buf[5] = 0x00;
    buf[6] = '4';
    buf[7] = 0x00;
    buf[8] = '5';
    buf[9] = 0x00;
    buf[10] = '6';
    buf[11] = 0x00;

    taskPool.AddTask(task, buf, 2);
    taskPool.AddTask(task, buf + 2, 2);
    taskPool.AddTask(task, buf + 4, 2);
    taskPool.AddTask(task, buf + 6, 2);
    taskPool.AddTask(task, buf + 8, 2);
    taskPool.AddTask(task, buf + 10, 2);

    //taskPool.AddTask(task1, buf, 2);

    pause();

    taskPool.UnInit();

    //cb(buf, 0, &x);


    return 0;
}


