#ifndef __RM_CODE_PRO_QUICK_TRANS_DEFINE_H__
#define __RM_CODE_PRO_QUICK_TRANS_DEFINE_H__

typedef enum _ProQuickTransReadMode {
    PRO_TRANS_READ_ORDER,   //顺序读取
    PRO_TRANS_READ_OLDEST,  //读取最旧数据
    PRO_TRANS_READ_NEWEST   //读取最新数据
} ProQuickTransReadMode_e;

typedef struct _ProQuickTransReadIndex {
    xint32_t index;
    xuint32_t varifyID;
    xint32_t ID;
} ProQuickTransReadIndex_t;


namespace RM_CODE 
{
    typedef struct _ProQuickTransCFG {
        xint32_t id;
        xint32_t size;
    } ProQuickTransCFG_t;
}
#endif
