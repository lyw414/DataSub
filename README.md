# DataSub
## 功能说明
linux 进程间数据订阅 订阅者提供数据缓存服务与任务队列

## 使用说明
编译后 请引用 DataSub.h 与 libDataSub.so

## 订阅数据
继承 SubScribeBase 实现订阅回调与超时回调即可
    #include "DataSub.h"
    class SubScribe : public SubScribeBase {
    ......
    }

## 发布数据
继承 PublishBase  实现发布超时回调即可
    #include "DataSub.h"
    class Publish: public PublishBase {
    ......
    }

