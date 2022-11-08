#ifndef SSSSSSSSSSSSSSSSSSSSS_X
#define SSSSSSSSSSSSSSSSSSSSS_X
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

typedef struct  _GpsInfo {
    char dd[512];
} GpsInfo_t;

typedef struct  _GpsAgreementData{
    char dd[512];
} GpsAgreementData_t;

#endif
