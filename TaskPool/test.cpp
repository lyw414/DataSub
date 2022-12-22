#include "TaskPoolAPI.h"  
#include <stdlib.h>

void Show(void * data, int len, void * user)
{
    printf("%p %d %p\n", data, len, user);
}

int main()
{
    RM_CBB_TaskPoolCreate(1024, 2);
    TaskHandle h = RM_CBB_RegisterNormalTask(Show, NULL, NULL);
    RM_CBB_AddTask(h, NULL, 20);

    pause();
    return 0;
}
