#include "TaskPoolAPI.h"

int main()
{
    RM_CBB_TaskPoolInit(1024, 4);
    RM_CBB_TaskPoolUnInit();
    return 0;
}

