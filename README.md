# DataSub
## 功能说明
linux 进程间数据订阅 订阅者提供数据缓存服务与任务队列

## 使用说明
编译后 请引用 DataSub.h 与 libDataSub.so

## 订阅数据
继承SubscribeTaskBase  
异步任务模式 需要实现任务回调

## 发布数据
继承PublishTaskBase  

# 模块
## IPC
启动模式待定；易用性上不应使用的单例，但是增加了引用者的维护（不可靠手段）；使用单例之后，易用性变差，定义与资源一一绑定，增加额外资源需要增加相应定义，不存在引用者问题.
