#ifndef __RM_CODE_FIX_LEN_LIST_FILE_HPP_
#define __RM_CODE_FIX_LEN_LIST_FILE_HPP_

#include "type.h"
#include <stdlib.h>

namespace RM_CODE
{

    template <typename T>
    class FixLenList
    {
    public:
        typedef struct _ListNode {
            struct _ListNode * pre;
            struct _ListNode * next;
            xint32_t st;
            T data;
        } ListNode_t;

        class iterator {
        private:
            friend class FixLenList;
            ListNode_t * m_node;

        public:
            iterator()
            {
                m_node = NULL;
            }

            iterator(const iterator & it)
            {
                m_node = it.m_node;
            }

            iterator(ListNode_t * node)
            {
                m_node = node;
            }

            ~iterator()
            {
                m_node = NULL;
            }

            iterator & operator ++()
            {
                if (m_node != NULL)
                {
                    m_node = m_node->next;
                }
                return *this;
            }


            iterator & operator ++(int)
            {
                if (m_node != NULL)
                {
                    m_node = m_node->next;
                }
                return *this;
            }

            iterator & operator --()
            {
                if (m_node != NULL)
                {
                    m_node = m_node->pre;
                }
                return *this;
            }

            //iterator & operator --(int)

            bool operator != (const iterator & it)
            {
                if (m_node == it.m_node)
                {
                    return false;
                }
                else
                {
                    return true;
                }
            }


            bool operator == (const iterator & it)
            {
                if (m_node == it.m_node)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }

            iterator & operator = (const iterator & it)
            {
                m_node = it.m_node;
                return *this;
            }
            
            T & operator * ()
            {
                return m_node->data;
            }
        };

    private:
        ListNode_t * m_begin;

        ListNode_t * m_end;

        ListNode_t * m_free;

        xuint32_t m_size;

        xuint32_t m_maxSize;

        xint32_t m_init;


    public:
        FixLenList()
        {
            m_begin = NULL;

            m_end = NULL;

            m_free = NULL;

            m_maxSize = 0;

            m_size = 0;

            m_init = 0;
        }


        xint32_t Reset(xuint32_t maxSize)
        {
            UnInit();
            Init(maxSize);
        }

        xint32_t Init(xuint32_t maxSize)
        {
            if (m_init == 1)
            {
                return -1;
            }

            m_init = 1;

            ListNode_t * ptr;

            ListNode_t * current;

            m_maxSize = maxSize;

            for (int iLoop = 0; iLoop < m_maxSize; iLoop++)
            {
                ptr = new ListNode_t;

                if (iLoop != 0)
                {
                    current->next = ptr;
                    ptr->pre = current;
                    ptr->next = NULL;
                    current = ptr;
                }
                else
                {
                    m_free = ptr;
                    current = ptr;
                    ptr->pre = NULL;
                    ptr->next = NULL;
                }
            }

            return 0;
        }
        
        xint32_t UnInit()
        {
            if (m_init == 0)
            {
                return 0;
            }

            m_init = 0;

            ListNode_t * ptr;

            ListNode_t * next;

            for (ptr = m_begin; ptr != NULL; )
            {
                next = ptr->next; 
                delete ptr;
                ptr = next;
            }

            for (ptr = m_free; ptr != NULL; )
            {
                next = ptr->next; 
                delete ptr;
                ptr = next;
            }
            
            m_begin = NULL;
            m_end = NULL;
            m_free = NULL;
            
            return 0;
        }

        ~FixLenList()
        {
            UnInit();
        }

        ListNode_t * Push_back(const T & value)
        {

            ListNode_t * node = NULL;

            if (NULL == m_free) 
            {
                return NULL;
            }
            else
            {
                node = m_free;

                node->st = 1;

                m_free = node->next;

                if (m_free != NULL)
                {
                    m_free->pre = NULL;
                }

                node->data = value;

                if (m_end != NULL)
                {
                    node->pre = m_end;
                    node->next = NULL;
                    m_end->next = node;
                    m_end = node;

                }
                else
                {
                    m_begin = m_end = node;
                    node->pre = NULL;
                    node->next = NULL;
                }

                m_size++;
            }

            return node;
        }

        ListNode_t * Push_front(const T & value)
        {
            ListNode_t * node = NULL;

            if (NULL == m_free) 
            {
                return NULL;
            }
            else
            {
                node = m_free;

                node->st = 1;

                m_free = node->next;

                if (m_free != NULL)
                {
                    m_free->pre = NULL;
                }

                node.data = value;

                if (m_begin != NULL)
                {
                    node->next = m_begin;
                    node->pre = NULL;
                    m_begin->pre = node;
                    m_begin = node;
                }
                else
                {
                    m_begin = m_end = node;
                    node->pre = NULL;
                    node->next = NULL;
                }

                m_size++;
            }

            return node;
        }

        ListNode_t * Front(T & out)
        {
            if (m_begin != NULL)
            {
                out = m_begin->data;
            }
            return m_begin;
        }

        ListNode_t * Back(T & out)
        {
            if (m_end != NULL)
            {
                out = m_end->data;
            }
            return m_end;
        }

        xint32_t Node(ListNode_t * node, T & out)
        {
            if (node != NULL && node->st == 1)
            {
                out = node->data;
            }
            return 0;
        }



        xint32_t Pop_front()
        {
            ListNode_t * ptr;
            if (m_begin != NULL)
            {
                ptr = m_begin;
                ptr->st = 0;
                m_begin = m_begin->next;
                if (m_begin == NULL)
                {
                    m_end = NULL;
                }
                else
                {
                    m_begin->pre = NULL;
                }

                if (m_free != NULL)
                {
                    ptr->pre = NULL;
                    ptr->next = m_free;
                    
                    m_free->pre = ptr;
                    m_free = ptr;
                }
                else
                {
                    ptr->pre = NULL;
                    ptr->next = NULL;
                    m_free = ptr;
                }

                m_size--;
                return m_size;
            }
            else
            {
                return -1;
            }

        }

        xint32_t Pop_back()
        {
            ListNode_t * ptr = NULL;
            if (m_end != NULL) 
            {
                ptr = m_end;
                ptr->st = 0;
                m_end = m_end->pre;

                if (m_end == NULL)
                {
                    m_begin = NULL;
                }
                else
                {
                    m_end->next = NULL;
                }

                if (m_free != NULL)
                {
                    ptr->pre = NULL;
                    ptr->next = m_free;

                    m_free->pre = ptr;
                    m_free = ptr;
                }
                else
                {
                    ptr->pre = NULL;
                    ptr->next = NULL;
                    m_free = ptr;
                }
                m_size--;
                return m_size;
            }
            else
            {
                return -1;
            }
        }


        iterator Begin()
        {
            return iterator(m_begin);
        }

        iterator End()
        {
            return iterator(NULL);
        }

        ListNode_t * Erase(ListNode_t * node)
        {
            ListNode_t * pre;
            ListNode_t * next;

            if (node != NULL && node->st == 1)
            {
                node->st = 0;
                pre = node->pre;
                next = node->next;

                if (pre != NULL)
                {
                    pre->next = next;
                }
                else if (node == m_begin)
                {
                    m_begin = next;
                    if (m_begin != NULL)
                    {
                        m_begin->pre = NULL;
                    }

                }

                if (next != NULL)
                {
                    next->pre = pre;
                }
                else if (m_end == node)
                {
                    m_end = pre;
                    if (m_end != NULL)
                    {
                        m_end->next = NULL;
                    }
                }


                if (m_free != NULL)
                {
                    node->next = m_free;
                    node->pre = NULL;

                    m_free->pre = node;
                    m_free = node;
                }
                else
                {
                    node->next = NULL;
                    node->pre = NULL;

                    m_free = node;
                }

                node = next;
            }

            return node;
        }

        iterator & Erase(iterator & it)
        {
            
            ListNode_t * pre;
            ListNode_t * next;


            if (it.m_node != NULL && it.m_node->st == 1)
            {
                pre = it.m_node->pre;
                next = it.m_node->next;
                
                if (pre != NULL)
                {
                    pre->next = next;
                }
                else
                {
                    m_begin = next;
                    if (m_begin != NULL)
                    {
                        m_begin->pre = NULL;
                    }

                }

                if (next != NULL)
                {
                    next->pre = pre;
                }
                else
                {
                    m_end = pre;
                    if (m_end != NULL)
                    {
                        m_end->next = NULL;
                    }
                }


                if (m_free != NULL)
                {
                    it.m_node->next = m_free;
                    it.m_node->pre = NULL;

                    m_free->pre = it.m_node;
                    m_free = it.m_node;
                }
                else
                {
                    it.m_node->next = NULL;
                    it.m_node->pre = NULL;

                    m_free = it.m_node;
                }

                it.m_node = next;
            }

            return it;
        }

        xint32_t Size()
        {
            return m_size;
        }


        bool Full()
        {
            if (m_free == NULL)
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        bool Empty()
        {
            if (m_begin == NULL)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    };
}
#endif
