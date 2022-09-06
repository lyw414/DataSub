#ifndef __LYW_CODE_DATA_SUB_H_FILE__
#define __LYW_CODE_DATA_SUB_H_FILE__

#include "DataSubFunc.hpp"
#include "TaskPool.hpp"
#include "RWBlockArray_IPC.hpp"
#include "ProSubTable.hpp"
#include "ThreadSubTable.hpp"

#include <string>


namespace LYW_CODE
{
    class DataSub
    {
    private:
        typedef struct _ShareInfo {
            TaskPool * task;

            ThreadSubTable * thSub;

            ProSubTable * proSub;

            RWBlockArray_IPC * ipc;
            //0 终止 1 运行 2 终止中
            int recvST;

        } ShareInfo_t;

    private:
        key_t m_key;

        std::string m_name;

        pthread_mutex_t m_lock;

        ShareInfo_t * m_param;

        pthread_t m_thread;

        pthread_attr_t m_attr;

        int m_cliIndex;

        int m_proID;
        


    private:

        //IPC 接收线程
        static void * RecvFromIPC_ (void * ptr);

        void RecvFromIPC ();


        bool IsReadFinished(void * msg);

        bool IsNeedData(void * msg, unsigned int size, void * param);

        void SetBit(unsigned char * bitMap, int index, unsigned char setValue);


        int GetBit(unsigned char * bitMap, int index);

        void BlockHandle(void * msg, unsigned int lenOfMsg);

        void DoThreadHandleFunc(void * msg, unsigned int lenOfMsg);

    public:
        DataSub();

        int Init(const char * name, int threadNum = 8);

        int UnInit();

        int Publish(int msgID, void * msg, int lenOfMsg);

        int Subcribe(int msgID, Function3<void(void *, unsigned int, void *)> handleCB, void * userParam, int mode, int timeout);

        int Response(int publishInfo, void * msg, int lenOfMsg);

        int SubcribeResponse(int msgID, int HandleMsg, int timeout);

        int UnSubcribe(int msgID);

        int UnSubcribeResponse(int msgID);
    };

}
#endif
