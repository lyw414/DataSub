#ifndef __LYW_CODE_SIMPLE_HANDLE_FUNC_HPP__
#define __LYW_CODE_SIMPLE_HANDLE_FUNC_HPP__
#include <functional>
namespace LYW_CODE
{
    template <typename T>
    class DataSubHandleFunc;


    template <typename Ret, typename ... Args>
    class DataSubHandleFunc<Ret(Args...)>
    {
    private:
        class None {};
        typedef Ret(None::*func_t)(Args...);
        func_t m_func;
        None * m_obj;

    public:
        template <typename OBJ>
        DataSubHandleFunc(Ret(OBJ::*func)(Args...), OBJ * obj) {
            m_func = (func_t)func;
            m_obj = (None *)obj ;
        }

        Ret operator () (Args ... args)
        {
            return  (m_obj->*m_func)(args...);
        }

    };

}

#endif
