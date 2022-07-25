#ifndef __LYW_CODE_DATA_SUB_H_FILE__
#define __LYW_CODE_DATA_SUB_H_FILE__
namespace LYW_CODE
{
    class DataSub
    {
    private:

    public:
        DataSub(const char * name);

        int Create(key_t key);
        
        int Destroy();

        int Start(int threadCount, int taskCount);

        int Stop(int threadCount, int taskCount);

        int Publish(int msgID, void * msg, int lenOfMsg);

        int Subcribe(int msgID, HandleMsg, int mode, int timeot);

        int Response(int PublishInfo, void * msg, int lenOfMsg);

        int SubcribeResponse(int msgID, HandleMsg, int timeout);

        int UnSubcribe(int msgID);

        int UnSubcribeResponse(int msgID);
    };
}
#endif
