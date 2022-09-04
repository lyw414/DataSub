#ifndef __LYW_CODE_PRO_TRANS_FILE_HPP__
#define __LYW_CODE_PRO_TRANS_FILE_HPP__
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#include <list>


namespace LYW_CODE
{
    class ProTrans {
    private:
        typedef struct _CFGList{
            int type;
            int maxRecordNum;
            int blockSize;
        } CFGList_t;

        typedef enum _NodeST {
            NODE_ST_RR, //可读
            NODE_ST_WW, //待写
            NODE_ST_WR, //写完成
        } NodeST_e;

        typedef struct _NodeInfo {
            //节点size
            int size;
            //读节点开始索引
            int r_B;
            //读结束节点索引
            int r_E;
            //写节点开始索引
            int w_B;
            //节点数据长度
            unsigned int nodeSize;
        } NodeInfo_t;

        typedef struct _Node {
            int len;
            int index;
            //读计数 读仅仅维护该字段
            int rdRecord;
            //索引使用 表示节点是否过期 过期节点 则会从可读节点开始读
            int nodeID;
            //节点状态 
            NodeST_e st;
        } Node_t;

        typedef struct _CacheBufInfo {
            int size;
        } CacheBufInfo_t;

        typedef struct _TypeTable {
            pthread_mutex_t lock;     
            pthread_mutexattr_t lockAttr;

            pthread_mutex_t lockCond;     
            pthread_mutexattr_t lockCondAttr;
            pthread_cond_t cond;
            pthread_condattr_t condAttr;
            
            //索引节点详情
            NodeInfo_t nodeInfo;
            Node_t * node;
            
            //缓存详情
            CacheBufInfo_t cacheInfo;
            unsigned char * cache;
        } TypeTable_t;

        typedef struct _CacheMap {
            int totalSize;
            //mapSize
            int size;
            //就绪状态 0 未就绪 1 就绪
            int st;
            //type 为index
            TypeTable_t ** typeTable;
        } CacheMap_t;

    private:
        pthread_mutex_t m_lock;

        key_t m_key;

        std::list<CFGList_t> m_cfg;

        int m_shmID;

        CacheMap_t * m_cacheMap;
        
        //初始化状态 0 失败 1 成功
        int m_st;
    
    public:
        typedef struct _IndexBlock {
            unsigned int id;
            int index;
        } IndexBlock_t;


    private:
        inline int NextIndex(int index, int size)
        {
            int next = index + 1;
            if (next < size)
            {
                return next;
            }
            else
            {
                return 0;
            }
        }

        inline int PreIndex(int index, int size)
        {
            if (index != 0)
            {
                return index - 1;
            }
            else
            {
                return size -1;
            }
        }


    public:
        ProTrans()
        {
            m_st = 0;

            m_key = 0x00;

            m_shmID = -1;

            ::pthread_mutex_init(&m_lock, NULL);
        }

        /**
         * @brief                       添加交换数据区配置 添加完成后使用 Init 生效配置
         * @param[in]type               数据类型，且为数组下标键值
         * @param[in]maxRecordNum       最大缓存记录数量，超出则会覆盖
         * @param[in]cacheBufferSize
         */
        int AddTransCFG(int type, int maxRecordNum, int blockSize)
        {
            CFGList_t cfg;

            cfg.type = type;
            cfg.maxRecordNum = maxRecordNum;
            cfg.blockSize = blockSize;


            ::pthread_mutex_lock(&m_lock);
            m_cfg.push_back(cfg);
            ::pthread_mutex_unlock(&m_lock);

            return 0;
        }
        
        /**
         * @brief                       生效添加的配置 仅第一次加载有效
         *
         * @param[in] key               共享缓存键值
         * @return                      0 成功 其余失败 
         */
        int Init(key_t key)
        {
            m_key = key;

            int retryTimes = 5;

            int totalSize = 0;

            int maxType = -1;

            unsigned char * ptr = NULL;

            TypeTable_t * typeTable = NULL;
            
            std::list<CFGList_t>::iterator it;

            std::list<CFGList_t> cfg;
            
            totalSize += sizeof(CacheMap_t);

            ::pthread_mutex_lock(&m_lock);
            cfg = m_cfg;
            ::pthread_mutex_unlock(&m_lock);

            for (it = cfg.begin(); it != cfg.end(); it++)
            {
                if (it->type > maxType)
                {
                    maxType = it->type;
                }
                
                totalSize += sizeof(TypeTable_t);

                totalSize += it->maxRecordNum * sizeof(Node_t);

                totalSize += it->blockSize;
            }

            totalSize += (maxType + 1) * sizeof(TypeTable_t *);

            if (m_st == 1)
            {
                //已经初始化
                return -1;
            }

            m_shmID = ::shmget(m_key, totalSize, 0666 | IPC_CREAT | IPC_EXCL);
            if (m_shmID >= 0)
            {
                //创建成功 进行初始化
                if ((m_cacheMap = (CacheMap_t *)::shmat(m_shmID, NULL, 0)) == NULL)
                {
                    printf("(new shm) Shmat Failed! %d\n", errno);
                    return -3;
                }
                
                //初始化
                memset(m_cacheMap, 0x00, totalSize);
                m_cacheMap->st = 0;
                m_cacheMap->totalSize = totalSize;
                m_cacheMap->size = maxType + 1;
                
                ptr = (unsigned char *)m_cacheMap;
                
                ptr += sizeof(CacheMap_t);

                m_cacheMap->typeTable = (TypeTable_t **)ptr;

                ptr += sizeof(TypeTable_t *) * m_cacheMap->size;


                for (it = cfg.begin(); it != cfg.end(); it++)
                {
                    m_cacheMap->typeTable[it->type] = (TypeTable_t *)ptr;
                    //初始化 
                    typeTable = m_cacheMap->typeTable[it->type];

                    //互斥量
                    pthread_mutexattr_init(&typeTable->lockAttr);

                    pthread_mutexattr_setpshared(&typeTable->lockAttr, PTHREAD_PROCESS_SHARED);

                    pthread_mutex_init(&typeTable->lock, &typeTable->lockAttr);
                    //条件变量锁
                    pthread_mutexattr_init(&typeTable->lockCondAttr);
                    pthread_mutexattr_setpshared(&typeTable->lockCondAttr, PTHREAD_PROCESS_SHARED);

                    pthread_mutex_init(&typeTable->lockCond, &typeTable->lockCondAttr);


                    //条件变量
                    pthread_condattr_init(&typeTable->condAttr);

                    pthread_condattr_setpshared(&typeTable->condAttr, PTHREAD_PROCESS_SHARED);
                    //进程共享
                    pthread_condattr_setpshared(&typeTable->condAttr, PTHREAD_PROCESS_SHARED);
                    //时钟属性
                    pthread_condattr_setclock(&typeTable->condAttr, CLOCK_MONOTONIC);

                    pthread_cond_init(&typeTable->cond, &typeTable->condAttr);
                    
                    //节点索引表详情
                    typeTable->nodeInfo.size = it->maxRecordNum + 3;
                    typeTable->nodeInfo.r_B = 0;
                    typeTable->nodeInfo.r_E = 1;
                    typeTable->nodeInfo.w_B = 2;
                    typeTable->nodeInfo.nodeSize = it->blockSize;

                    //缓存表索引详情
                    typeTable->cacheInfo.size = it->blockSize * it->maxRecordNum;
                    ptr += sizeof(TypeTable_t);

                    //node Array   
                    typeTable->node = (Node_t *)ptr;
                    ptr += sizeof(Node_t) * typeTable->nodeInfo.size;
                    for(int iLoop1 = 0; iLoop1 < typeTable->nodeInfo.size; iLoop1++)
                    {
                        typeTable->node[iLoop1].st = NODE_ST_WW;
                        typeTable->node[iLoop1].index = iLoop1 * typeTable->nodeInfo.nodeSize;
                    }

                    //cache Buff
                    typeTable->cache = ptr;
                    ptr += typeTable->cacheInfo.size;
                }

                m_cacheMap->st = 1;
            }
            else
            {
                //等待连接共享缓存初始化当前buffer
                if ((m_shmID = ::shmget(m_key, 0, 0)) < 0)
                {
                    //链接失败
                    printf("(Exist shm) ShmGet Failed! %d\n", errno);
                    return -3;
                }

                if ((m_cacheMap = (CacheMap_t *)::shmat(m_shmID, NULL, 0)) == NULL)
                {
                    printf("(Exist shm) Shmat Failed! %d\n", errno);
                    return -3;
                }

                //检查共享缓存就绪标识 等待共享缓存初始化完成
                while (retryTimes > 0)
                {
                    if (m_cacheMap->st == 1)
                    {
                        break; 
                    }
                    ::usleep(500000);
                    retryTimes--;
                    
                }

                if (retryTimes <= 0)
                {
                    printf("Shm Not Ready! Timeout!\n");
                    m_st = 0;
                    return -2;
                }
            }

            m_st = 1;

            return 0;
        }
        
        /**
         * @breif                       添加一则数据
         *
         * @param[in] type              数据类型
         * @param[in] data              数据
         * @param[in] lenOfData         数据长度
         * @return                      0 成功 其余失败
         */
        int Write(int type, unsigned char * data,  int lenOfData)
        {
            TypeTable_t * typeTable = NULL;

            int tmp_rb;
            int tmp_re;
            int tmp_wb;

            int index = 0;
            int next = 0;
            int next_next = 0;
            int pre = 0;

            if (m_st == 0)
            {
                return -1;
            }
            
            if (type >= m_cacheMap->size || (typeTable = m_cacheMap->typeTable[type]) == NULL || typeTable->nodeInfo.nodeSize < lenOfData)
            {
                return -2;
            }

            while (true) 
            {
                ::pthread_mutex_lock(&typeTable->lock);
                index = typeTable->nodeInfo.w_B;
                pre = PreIndex(index, typeTable->nodeInfo.size);
                next = NextIndex(index, typeTable->nodeInfo.size);
                next_next = NextIndex(next, typeTable->nodeInfo.size);

                tmp_rb = typeTable->nodeInfo.r_B;
                tmp_re = typeTable->nodeInfo.r_E;
                tmp_wb = typeTable->nodeInfo.w_B;

                if(next == typeTable->nodeInfo.r_B && next_next == typeTable->nodeInfo.r_E)
                {
                    //写节点已满
                    ::pthread_mutex_unlock(&typeTable->lock);
                    ::usleep(10000);
                    continue;
                }

                
                //可写节点后移
                typeTable->nodeInfo.w_B = next;
                typeTable->node[next].st = NODE_ST_WW;
                typeTable->node[next].nodeID++;

                //覆盖读节点
                if (typeTable->nodeInfo.r_B == next)
                {
                    typeTable->nodeInfo.r_B = next_next;
                    typeTable->node[next_next].st = NODE_ST_WW;
                    typeTable->node[next_next].nodeID++;
                }
                typeTable->node[pre].st = NODE_ST_WW;
                typeTable->node[pre].nodeID++;
                ::pthread_mutex_unlock(&typeTable->lock);
                break;
            }

            //自旋等待读完成
            while(true)
            {
                if (typeTable->node[pre].rdRecord == 0)
                {
                    break;
                }
                ::usleep(10000);
            }

            //写数据
            ::memcpy(typeTable->cache + typeTable->node[pre].index, data, lenOfData);
            typeTable->node[pre].len = lenOfData;

            //提交index
            ::pthread_mutex_lock(&typeTable->lock);
            typeTable->node[pre].st = NODE_ST_RR;
            int t = 0;
            while (true)
            {
                if (index == typeTable->nodeInfo.w_B)
                {
                    break;
                }
                
                if (pre == typeTable->nodeInfo.r_E && typeTable->node[pre].st == NODE_ST_RR)
                {
                    typeTable->nodeInfo.r_E = index;
                    pre = index;
                    index = next;
                    next = NextIndex(next, typeTable->nodeInfo.size);
                }
                else
                {
                    break;
                }
            }
            ::pthread_mutex_unlock(&typeTable->lock);

            //::pthread_cond_broadcast(&typeTable->cond);
            return 0;
        }

        /**
         * @brief                       读取一则数据
         *
         * @param[in] BH                记录上一次读取索引信息
         * @param[in] type              数据类型
         * @param[in] data              数据拷贝指针 
         * @param[in] sizeOfData        data的大小
         * @param[in] len               数据长度
         *
         * @return  >  0                数据长度 
         *          <= 0                错误码
         */
        int Read(IndexBlock_t * BH, int type, unsigned char * data,  int sizeOfData, int *len = NULL)
        {
            int index = 0;

            int next = 0;

            int res = 0;

            TypeTable_t * typeTable = NULL;

            if (m_st == 0 || BH == NULL)
            {
                return -1;
            }
            
            if (type >= m_cacheMap->size || (typeTable = m_cacheMap->typeTable[type]) == NULL)
            {
                return -2;
            }
            
            //阻塞读
            while(true)
            {
                ::pthread_mutex_lock(&typeTable->lock);
                if (BH->index < 0 || BH->index >= typeTable->nodeInfo.size || BH->index == typeTable->nodeInfo.r_B || typeTable->node[BH->index].nodeID != BH->id)
                {
                    //id 发生覆盖 读取最旧的数据
                    index = typeTable->nodeInfo.r_B;
                    BH->index = index;
                    BH->id = typeTable->node[index].nodeID;
                }
                else
                {
                    index = BH->index;
                }

                next = NextIndex(index, typeTable->nodeInfo.size);

                if (next == typeTable->nodeInfo.r_E)
                {
                    //没有新数据阻塞读
                    ::pthread_mutex_unlock(&typeTable->lock);

                    usleep(0);
                    //::pthread_mutex_lock(&typeTable->lockCond);
                    //::pthread_cond_wait(&typeTable->cond, &typeTable->lockCond);
                    //::pthread_mutex_unlock(&typeTable->lockCond);

                    //::pthread_cond_wait(&typeTable->cond, &typeTable->lock);
                    //::pthread_mutex_unlock(&typeTable->lock);
                    continue;
                }

                index = next;

                typeTable->node[index].rdRecord++;
                BH->index = index;
                BH->id = typeTable->node[index].nodeID;
                ::pthread_mutex_unlock(&typeTable->lock);
                break;
            }

            *len = typeTable->node[index].len;
            if (sizeOfData >=  typeTable->node[index].len)
            {
                ::memcpy(data, typeTable->cache +  typeTable->node[index].index,  typeTable->node[index].len);
            }
            else
            {
                res = -1;
            }

            ::pthread_mutex_lock(&typeTable->lock);
            typeTable->node[index].rdRecord--;
            ::pthread_mutex_unlock(&typeTable->lock);
            return res;
        }

        //释放共享缓存
        int Destroy(key_t key)
        {
            struct shmid_ds buf;
            int shmID = ::shmget(key, 0, IPC_EXCL);
            if (shmID > 0)
            {
                ::shmctl(shmID, IPC_RMID, &buf);
            }

            return 0;
        }

        //void ShowNode(Node_t * node, int size )
        //{
        //    printf("*************Begin***************\n")
        //    for(int iLoop = 0; iLoop < size; iLoop++)
        //    {
        //        printf("index %d ST  ") 
        //    }
        //    printf("*************End***************\n")
        //}
    };
}
#endif
