#ifndef __RM_CODE_CSTREAM_PRO_QUICK_TRANS_FILE_H__
#define __RM_CODE_CSTREAM_PRO_QUICK_TRANS_FILE_H__
#include <streamaxsdk.h>
#include <streamaxlog.h>

#include <map>

#include "ProQuickTransAPI.h"
#include "TaskPoolAPI.h"
namespace RM_CODE
{
    class ProQuickTransImp {
    private:
        class TransParamKey_t {
        public:
            void * data0;
            void * data1;
            
            bool operator < (const  TransParamKey_t & key ) const {
                if (this->data0 < key.data0)
                {
                    return true;
                }
                else if (this->data0 == key.data0)
                {
                    if (this->data1 < key.data1)
                    {
                        return true;
                    }
                }

                return false;
            }
        };

        typedef struct _TransParam{
            AsyncCallbackFunc func;
            void * data0;
            void * data1;
            xuint32_t dataType;
            ProQuickTransNodeIndex_t IN;
            void * taskHandle;
            xbyte_t * readData;
            xuint32_t sizeOfData;
        } TransParam_t;




    private:
        ::pthread_mutex_t m_lock;

        std::map <TransParamKey_t, TransParam_t *> m_transParam;
        ProQuickTransHandle m_proQuickTransHandle;


    private:
       void TransTimeReadTask(void * param, xint32_t lenOfParam, void * userParam);

    private:
        ProQuickTransImp();

    public:
        static ProQuickTransImp * GetInstance();
        
        /**
         * @brief                   注册回调透传任务
         *
         * @param[in] func          需要透转的回调
         * @param[in] data0         用户参数
         * @param[in] data1         用户参数
         * @param[in] dataType      数据类型
         * @param[in] interval      调度间隔
         * 
         * @return  >= 0            成功
         *          <  0            失败
         */
        xint32_t RegisterTransTask(AsyncCallbackFunc func, xpvoid_t data0, xpvoid_t data1, xuint32_t dataType, xuint32_t interval);

    };


}
#endif
