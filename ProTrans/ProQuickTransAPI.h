#ifndef __RM_CBB_API_PRO_QUICK_TRANS_API_FILE_H__
#define __RM_CBB_API_PRO_QUICK_TRANS_API_FILE_H__

#include <stdio.h>
#include <sys/types.h>
#include <map>

#include "streamaxcomdev.h"
#include "ProQuickTransDefine.h"

#define PRO_QUICK_TRANS_DEFINE_BEGIN() static const xint32_t g_proQuickTransCFG[] = {
#define PRO_QUICK_TRANS_DEFINE_DEFINE(key, ID, BlockCount, BlockSize) \
key,\
ID,\
BlockCount,\
BlockSize,
#define PRO_QUICK_TRANS_DEFINE_END() };



typedef RM_CODE::ProQuickTransNodeIndex_t ProQuickTransNodeIndex_t;


//typedef RM_CODE::ProQuickTrans * ProQuickTransHandle;
typedef void * ProQuickTransHandle;

//extern "C" {
    /**
     * @brief               基于g_proQuickTransCFG生成共享存配置 若对应键值共享内存未创建则创建，并获取对应键值的实例句柄
     * @param[in]key        键值
     * 
     * @return              失败 NULL 成功句柄
     */
    INTERFACE_API ProQuickTransHandle RM_CBB_ProQuickTransInit(key_t key);
    
    /**
     * @brief               释放句柄资源
     * @param[in]handle     RM_CBB_ProQuickTransInit 获取
     *
     */
    INTERFACE_API void RM_CBB_ProQuickTransUnInit(ProQuickTransHandle * h);
    
    /**
     * @brief               销毁共享缓存
     * @param[in]key        键值
     * 
     */
    INTERFACE_API void RM_CBB_ProQuickTransDestroy(key_t key);
    
    /**
     * @brief                   写数据
     *
     * @param[in]handle         资源句柄
     * @param[in]id             登记的数据ID
     * @param[in]data           待写入的数据
     * @param[in]lenOfData      待写入的数据长度
     * @param[in]timeout        超时时间 us
     *
     * @return  >= 0            成功
     *          <  0            失败 错误码 -1 写超时 -2 数据过大
     */
    INTERFACE_API xint32_t RM_CBB_ProQuickTransWrite(ProQuickTransHandle h, xint32_t id, xbyte_t * data, xint32_t lenOfData, xint32_t interval = 10000, xint32_t timeout = 0);
    
    
    /**
     * @brief                   读数据
     *
     * @param[in]handle         资源句柄
     * @param[in]IN             节点索引 控制读模式
     * @param[in]id             登记的数据ID
     * @param[out]data          读取数据的缓冲区
     * @param[in]sizeOfData     缓存区大小
     * @param[in]timeout        超时时间 us
     *
     * @return  >= 0            成功
     *          <  0            失败 错误码 -1 超时 -2 缓存池过小
     */
    INTERFACE_API xint32_t RM_CBB_ProQuickTransRead(ProQuickTransHandle h, ProQuickTransNodeIndex_t * IN, xint32_t id, xbyte_t * data, xint32_t sizeOfData, xint32_t interval = 10000, xint32_t timeout = 0);

    /**
     * @brief                   获取指定id类型数据的长度大小
     *
     * @param[in]id             数据类型ID
     *   
     * @return  >= 0            成功  
     *          <  0            失败 错误码
     */
    INTERFACE_API xint32_t RM_CBB_ProQuickTransDataSize(ProQuickTransHandle h, xint32_t id);
//}
#endif
