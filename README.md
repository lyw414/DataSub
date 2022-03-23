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
