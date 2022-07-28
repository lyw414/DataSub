#ifndef __LYW_CODE_DATA_SUB_FUNC_HPP__
#define __LYW_CODE_DATA_SUB_FUNC_HPP__
#define SubErrorCB SubHandleCB 

#include <stdlib.h>
namespace LYW_CODE
{
    template<typename T>
    class SubHandleCB;

    
    template<typename RES, typename ARGS1, typename ARGS2, typename ARGS3>
    class SubHandleCB <RES(ARGS1,ARGS2,ARGS3)>
    {
    private:
        class None {};
        typedef RES(None::*objFunc_t)(ARGS1, ARGS2, ARGS3);
        typedef RES(*stFunc_t)(ARGS1, ARGS2, ARGS3);
        None * m_obj;
        
        union {
            objFunc_t objFunc;
            stFunc_t stFunc;
        } m_func;


    public:
        template<typename OBJ>
        SubHandleCB(RES(OBJ::*func)(ARGS1, ARGS2, ARGS3), OBJ * obj)
        {
            m_func.objFunc = (objFunc_t)(func);
            m_obj = (None *)obj; 
        }


        SubHandleCB(RES(*func)(ARGS1, ARGS2, ARGS3))
        {
            m_obj = NULL;
            m_func.stFunc = (stFunc_t)(func);
        }

        RES operator() (ARGS1 args1, ARGS2 args2, ARGS3 args3)
        {
            if (m_obj != NULL)
            {
                return (m_obj->*(m_func.objFunc))(args1, args2, args3);
            }
            else
            {
                return m_func.stFunc(args1, args2, args3);
            }
        }
    };

}
#endif
