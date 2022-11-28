#ifndef __RM_CODE_COMM_DEF_FILE_HPP_
#define __RM_CODE_COMM_DEF_FILE_HPP_
namespace RM_CODE 
{
    typedef enum _ProQuickTranReadMode {
        PRO_READ_ORDER,                 //顺序读取
        PRO_READ_NEWEST_ONLY,           //读取最新
        PRO_READ_OLDEST_ONLY,           //读取最旧
    } ProQuickTranReadMode_e;

    typedef struct _ProQuickTransReadRecord {
        xint32_t index; 
        xuint32_t varifyID;
    } ProQuickTransReadRecord_t;

    typedef struct _ProQuickTransCFG {
        xint32_t id;
        xint32_t size;
    } ProQuickTransCFG_t;
}
#endif
