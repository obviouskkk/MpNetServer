## MpNetServer
#### 简介
- 一个简单的多进程网络服务器框架,加载so调用
#### 网络通信
- 父进程：处理和客户端的连接，进行数据的收发；接收数据放到接收队列、从发送队列提取数据发送
- 子进程：处理主动发起的连接
##### 接收队列
- 内存映射创建，每个子进程和父进程之间维护一个;
- 父进程将数据放到队列中，写pipe通知子进程，子进程处理；
- 父进程操作head指针，子进程操作tail指针，避免加锁
##### 发送队列
- 类似 接收队列

#### 通信管道
- 父子进程间维护两个管道，一个管道只能父进程写，子进程读；另一个相反；
- 父子进程通过管道通知有数据需要处理/发送；

#### ...
