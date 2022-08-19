#ifndef __LYW_CODE_DATA_SUB_FUNC_HPP__
#define __LYW_CODE_DATA_SUB_FUNC_HPP__
#define SubErrorCB SubHandleCB 

#define Function3 SubHandleCB 

#include <stdlib.h>
namespace LYW_CODE
{

    template<typename T>
    class Function2;

    template<typename RES, typename ARGS1, typename ARGS2>
    class Function2 <RES(ARGS1,ARGS2)>
    {
    private:
        class None {};
        typedef RES(None::*objFunc_t)(ARGS1, ARGS2);
        typedef RES(*stFunc_t)(ARGS1, ARGS2);
        None * m_obj;
        
        union {
            objFunc_t objFunc;
            stFunc_t stFunc;
        } m_func;


    public:
        template<typename OBJ>
        Function2(RES(OBJ::*func)(ARGS1, ARGS2), OBJ * obj)
        {
            m_func.objFunc = (objFunc_t)(func);
            m_obj = (None *)obj; 
        }


        Function2()
        {
            m_obj = NULL; 
            m_func.stFunc = NULL;
        }


        Function2(RES(*func)(ARGS1, ARGS2))
        {
            m_obj = NULL;
            m_func.stFunc = (stFunc_t)(func);
        }

        RES operator() (ARGS1 args1, ARGS2 args2)
        {
            if (m_obj != NULL)
            {
                return (m_obj->*(m_func.objFunc))(args1, args2);
            }
            else
            {
                return m_func.stFunc(args1);
            }
        }
        
        Function2 & operator= (void * ptr)
        {
            m_obj = NULL;
            m_func.stFunc = (stFunc_t)ptr;
            return *this;
        }

        Function2 & operator= (const Function2 & ptr)
        {
            m_obj = ptr.m_obj;
            m_func = ptr.m_func;
            return *this;
        }


        bool operator== (void * ptr)
        {
            return (m_func.stFunc == (stFunc_t)ptr);
        }

        bool operator!= (void * ptr)
        {
            return (m_func.stFunc != (stFunc_t)ptr);
        }



    };



    template<typename T>
    class Function1;

    template<typename RES, typename ARGS1>
    class Function1 <RES(ARGS1)>
    {
    private:
        class None {};
        typedef RES(None::*objFunc_t)(ARGS1);
        typedef RES(*stFunc_t)(ARGS1);
        None * m_obj;
        
        union {
            objFunc_t objFunc;
            stFunc_t stFunc;
        } m_func;


    public:
        template<typename OBJ>
        Function1(RES(OBJ::*func)(ARGS1), OBJ * obj)
        {
            m_func.objFunc = (objFunc_t)(func);
            m_obj = (None *)obj; 
        }


        Function1()
        {
            m_obj = NULL; 
            m_func.stFunc = NULL;
        }


        Function1(RES(*func)(ARGS1))
        {
            m_obj = NULL;
            m_func.stFunc = (stFunc_t)(func);
        }

        RES operator() (ARGS1 args1)
        {
            if (m_obj != NULL)
            {
                return (m_obj->*(m_func.objFunc))(args1);
            }
            else
            {
                return m_func.stFunc(args1);
            }
        }
        
        Function1 & operator= (void * ptr)
        {
            m_obj = NULL;
            m_func.stFunc = (stFunc_t)ptr;
            return *this;
        }

        Function1 & operator= (const Function1 & ptr)
        {
            m_obj = ptr.m_obj;
            m_func = ptr.m_func;
            return *this;
        }

        bool operator== (void * ptr)
        {
            return (m_func.stFunc == (stFunc_t)ptr);
        }

        bool operator!= (void * ptr)
        {
            return (m_func.stFunc != (stFunc_t)ptr);
        }



    };

    

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


        SubHandleCB()
        {
            m_obj = NULL;
            m_func.stFunc = NULL;
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

        SubHandleCB & operator= (void * ptr)
        {
            m_obj = NULL;
            m_func.stFunc = (stFunc_t)ptr;
            return *this;
        }

        SubHandleCB & operator= (const SubHandleCB & ptr)
        {
            m_obj = ptr.m_obj;
            m_func = ptr.m_func;
            return *this;
        }


        bool operator== (void * ptr)
        {
            return (m_func.stFunc == (stFunc_t)ptr);
        }

        bool operator!= (void * ptr)
        {
            return (m_func.stFunc != (stFunc_t)ptr);
        }

    };

}
#endif
