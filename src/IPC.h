namespace LYW_CODE
{
    /**
     * @brief           IPC服务
     */

    class IPC
    {
    private:
        IPC();
    public:
        ~IPC();
        static IPC * GetInstance();
    };
}
