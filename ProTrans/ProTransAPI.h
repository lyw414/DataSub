#ifndef __LYW_CODE_PRO_TRANS_API_FILE_H__
#define __LYW_CODE_PRO_TRANS_API_FILE_H__


#include <stdio.h>
#include "ProTrans.hpp"

extern "C" {
    int ProTransInit();
    int ProTransRead();
    int ProTransWrite();
}

#endif
