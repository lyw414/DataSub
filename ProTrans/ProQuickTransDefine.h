#ifndef __RM_CODE_COMM_DEF_FILE_HPP_
#define __RM_CODE_COMM_DEF_FILE_HPP_
namespace RM_CODE 
{
    typedef struct _ProQuickTransNodeIndex{
        xint32_t index; 
        xint32_t nodeID;
        xint32_t ID;
        //0 顺序读 1 最旧值 2 最新值
        xint32_t mode; 
    } ProQuickTransNodeIndex_t;

    typedef struct _ProQuickTransCFG {
        xint32_t id;
        xint32_t size;
    } ProQuickTransCFG_t;
}
#endif
