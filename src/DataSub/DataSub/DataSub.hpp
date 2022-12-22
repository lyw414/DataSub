#ifndef __RM_CODE_DATA_SUB_HPP_FILE__
#define __RM_CODE_DATA_SUB_HPP_FILE__
#include <sys/types.h>
#include <pthread.h>
#include <string>
#include <stdio.h>
#include <map>

#include "streamaxcomdev.h"
#include "streamaxlog.h"

#include "SimpleFunction.hpp"
#include "Hash.hpp"
#include "ProTrans.hpp"
#include "SubTable.hpp"
#include "TaskPool.hpp"
#include "ProInstance.hpp"

namespace RM_CODE
{
    class DataSub
    {
    private:
        typedef struct _SubParam {
            void * userParam;
            xint32_t taskHandle;
        } SubParam_t;

    private:
        pthread_mutex_t m_lock;

    private:
        xint32_t m_st;
        std::string m_ID;
        key_t m_key;
        ProTrans m_proTrans;
        SubTable m_subTable;
        TaskPool m_taskPool;
        xint32_t m_analysisTaskHandle;

    private:
        xint32_t IsInit()
        {
            if (m_st != 1)
            {
                return -1;
            }

            return 1;
        }

        /**
         * @brief                       检测是否需要当前块
         *
         * @param msg                   读取块
         * @param lenOfMsg              读取块的大小
         *
         * @return  true                需要块
         *          false               不需要块
         */
        bool IsReadBlockNeed(xbyte_t * msg, xint32_t lenOfMsg)
        {
            return true;
        }

        /**
         * @brief                       检测当前节点是否消费完成
         *
         * @param msg
         * @param lenOfMsg
         *
         * @return 
         */
        bool IsReadFinished(xbyte_t * msg, xint32_t lenOfMsg)
        {
            return true;
        }

        
        /**
         * @brief                       获取到完成完整的数据报文，使用完成后需要释放数据（共享数据策略）
         *
         * @param msg                   共享数据
         * @param lenOfMsg              数据长度
         * @param userParam             用户参数 - NULL
         */
        void ExcuteSubFunc(void * msg, xint32_t lenOfMsg, void * userParam)
        {

            DataSubInfo_t subInfo;
            SubParam_t * param = NULL;
            xint32_t msgHandle = 0;
            m_subTable.GetSubInfo(msgHandle, &subInfo);
            param = (SubParam_t *)subInfo.param;
            subInfo.handleFunc(msg, lenOfMsg, param->userParam);
            //线程完成读操作 释放进程中的缓存

        }
        
        /**
         * @brief                       解析原始块 --- 此处由协议模块解析，完成数据组包、拷贝、任务分发
         *
         * @param msg                   原始数据
         * @param lenOfMsg              数据长度
         * @param userParam             用户参数
         *
         */
        void AnalysisResourceReadBlock(void * msg, xint32_t lenOfMsg, void * userParam)
        {
            //调用协议模块进程拆包组包
            xint32_t msgIndex = -1;
            xint32_t msgHandle[32];
            int32_t lenOfTable;
            DataSubInfo_t subInfo;
            SubParam_t * subParam;


            //将参数保存至当前进程的缓存中 --- 生成订阅数据结构体
            xint32_t lenOfData = 0;
            
            //通过协议解析获取报文中 消息索引 使用消息索引获取线程订阅表（进程、线程订阅表共用一个索引值）
            m_subTable.ThrSubTable(msgIndex, msgHandle, 32*4, &lenOfTable);

            for (xint32_t iLoop = 0; iLoop < lenOfTable; iLoop++)
            {
                //线程订阅表中包含多个订阅该消息的订阅信息 保存消息句柄 msgHandle
                m_subTable.GetSubInfo(msgHandle[iLoop], &subInfo);
                subParam = (SubParam_t *)subInfo.param;

                //执行任务 -- 任务句柄可以保证线性执行或并行执行
                m_taskPool.ExcuteTask(subParam->taskHandle, Function3<void(void *, xint32_t, void *)> (&DataSub::ExcuteSubFunc, this), msg, lenOfData, NULL);
            }
        }



        /**
         * @brief                       进程唯一线程 读取订阅数据缓存并发布订阅任务
         *
         * @param ptr
         *
         * @return 
         */
        static void * ReadFromProTrans(void * ptr)
        {
            DataSub * self = (DataSub *)ptr;

            self->ReadFromProTrans_run();
            return NULL;
        }

        void ReadFromProTrans_run()
        {
            //检测是否需要启动该线程 线程是进程唯一的
            RM_CBB_LOG_ERROR("DATASUB","ID [%s] Start ReadFromProTrans!!!\n", m_ID.c_str());
            ProTransReadIndex_t readIndex = {0};
            xbyte_t * data = NULL;
            xbyte_t sizeOfData = m_proTrans.BlockSize();

            Function2<bool(xbyte_t *, xint32_t)> isNeedFunc(&DataSub::IsReadBlockNeed, this);
            Function3<void(void *, xint32_t, void *)> analysisResourceFunc(&DataSub::AnalysisResourceReadBlock, this);

            while(m_st == 1)
            {
                data = m_proTrans.Read(&readIndex, isNeedFunc, 2000);

                if (data != NULL)
                {
                    m_taskPool.ExcuteTask(m_analysisTaskHandle, analysisResourceFunc, data, sizeOfData, NULL);
                }
            }
            return;
        }




    public:
        DataSub()
        {
            ::pthread_mutex_init(&m_lock, NULL);
            m_st = 0;
        }

        /**
         * @brief                       初始化 
         *
         * @param ID                    资源ID 
         * @param blockNum              进程透传块个数
         * @param blockSize             进程透传块长度
         * @param workThreadNum         工作线程数量
         *
         * @return  >=  0               成功 0 创建成功 1 链接成功（链接模式 blockNum blockSize workThreadNum使用创建时候的值）
         *          <   0               失败 错误码
         */
        xint32_t Init(const std::string & ID, xint32_t blockNum = 0, xint32_t blockSize = 0, xint32_t workThreadNum = 0)
        {
            m_ID = ID;
            m_key = Hash::APHash(m_ID.c_str(), m_ID.length());
            xint32_t ret = 0;

            ::pthread_mutex_lock(&m_lock);
            if (m_st == 0)
            {
                RM_CBB_LOG_ERROR("DATASUB","key [%d] Init Begin.......\n", m_key);
                m_st = 1;
                //链接进程透传
                Function2<bool(xbyte_t *, xint32_t)> isFinishedFunc(&DataSub::IsReadFinished, this);
                ret = m_proTrans.Connect(m_key, blockNum, blockSize, isFinishedFunc);

                //链接订阅表
                m_subTable.Connect(m_key + 1);
                
                //链接内存池

                //链接任务池
                m_taskPool.Connect(ID, workThreadNum);
                m_analysisTaskHandle = m_taskPool.Register();

                //链接协议
                
                RM_CBB_LOG_ERROR("DATASUB","key [%d] Init finished.......\n", m_key);
            }
            ::pthread_mutex_unlock(&m_lock);

            return ret;
        }

        xint32_t UnInit()
        {
            ::pthread_mutex_lock(&m_lock);
            if (m_st == 1)
            {
                m_st = 0;

                m_proTrans.DisConnect();

                m_subTable.DisConnect();

            }
            ::pthread_mutex_unlock(&m_lock);
            return 0;
        }

        xint32_t Destory(key_t key)
        {
            return 0;
        }

        xint32_t Destory()
        {
            m_proTrans.Destroy();

            m_subTable.Destroy();

            return 0;
        }

        xint32_t RegisterMsg(xint32_t msgID)
        {
            xint32_t msgHandle = -1;
            if (IsInit() < 0)
            {
                RM_CBB_LOG_ERROR("DATASUB","key [%d] not Init!!!\n", m_key);
            }

            msgHandle = m_subTable.MsgRegister(msgID);

            if (msgHandle < 0)
            {
                RM_CBB_LOG_ERROR("DATASUB","key [%d] MSG [%d] Regist Error\n", m_key, msgID);
            }

            return msgHandle;
        }

        xint32_t UnRegisterMsg(xint32_t msgHandle)
        {
            if (IsInit() < 0)
            {
                RM_CBB_LOG_ERROR("DATASUB","key [%d] not Init!!!\n", m_key);
            }
            DataSubInfo_t subInfo; 

            //此处应该防止重复释放资源 .....
            m_subTable.GetSubInfo(msgHandle, &subInfo);
            ::free(subInfo.param);

            return m_subTable.MsgUnRegister(msgHandle);
        }

        xint32_t Publish(xint32_t msgHandle, void * msg, xint32_t lenOfMsg)
        {
            xint32_t lenOfTable = 0;
            xbyte_t head[1024] = {0};
            //根据协议完成协议头的封装 -- 至少要有进程订阅表（供其他订阅了的进程获取消息）
            //协议还应支持拆包、组包的等内容
            //暂时不做实现后续填充
            m_subTable.ProSubTable(msgHandle, (xbyte_t *)msg + 4, lenOfMsg - 4, &lenOfTable);
            *((xint32_t *)msg) = lenOfTable;



            //发送消息
            m_proTrans.Write(msg, lenOfMsg);
            return 0;
        }

        xint32_t Subcribe(xint32_t msgHandle, Function3<void(void *, xint32_t, void *)> handleFunc, void * userParam)
        {
            //并行订阅 - 并发处理
            SubParam_t * param = (SubParam_t *)malloc(sizeof(SubParam_t));
            xint32_t taskHandle = 0;
            taskHandle = m_taskPool.Register();

            param->userParam = userParam;
            param->taskHandle = taskHandle;

            DataSubInfo_t subInfo; 
            subInfo.param = param;
            subInfo.handleFunc = handleFunc;
            m_subTable.SetSubInfo(msgHandle, &subInfo);
            return 0;
        }
         
        xint32_t Subcribe_ser(xint32_t msgHandle, Function3<void(void *, xint32_t, void *)> handleFunc, void * userParam)
        {
            //串行订阅 - 串行处理
            return 0;
        }
    };

}
#endif
