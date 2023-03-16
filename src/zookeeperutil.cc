#include "zookeeperutil.h"
#include "mprpcapplication.h"
#include <iostream>
#include <semaphore.h>

// 全局的watcher观察器;zkserver给zkclient的通知  连接zookeeper的回调函数
void gloval_watcher(zhandle_t *zh, int type,
                    int state, const char *path, void *watcherCtx)
{
    if (type == ZOO_SESSION_EVENT)  // 回调的消息类型是和会话相关的消息类型
    {
        if (state == ZOO_CONNECTED_STATE)   // zkclient和zkserver连接成功
        {
            sem_t *sem = (sem_t *)zoo_get_context(zh);  // 从之前绑定的句柄中获取信号量
            sem_post(sem);                              // 给信号量资源加一，解除Start函数的sem阻塞
        }
    }
}

ZkClient::ZkClient() : m_zhandle(nullptr)
{
}

ZkClient::~ZkClient()
{
    if (m_zhandle != nullptr)
    {
        zookeeper_close(m_zhandle); // 关闭句柄，释放资源 MySQL_Conn
    }
}

// 连接zkserver
void ZkClient::Start()
{
    std::string host = MprpcApplication::GetInstance().GetConfig().Load("zookeeperip");
    std::string port = MprpcApplication::GetInstance().GetConfig().Load("zookeeperport");
    std::string connstr = host + ":" + port; // 格式是固定的

    /* 参数：1.IP地址和端口号、2.回调函数、3.超时时间
        zookeepe_mt:多线程版本
        zookeeper的API客户端程序提供了三个线程
        1.API调用线程
        2.网络I/O线程   pthread_create 底层使用的poll
        3.watcher回调线程
    */
    // 这个只是给句柄开辟内存空间，并没有连接上zkserver呢，要等待回调函数的判断
    m_zhandle = zookeeper_init(connstr.c_str(), gloval_watcher, 30000, nullptr, nullptr, 0);
    if (m_zhandle == nullptr)
    {
        std::cout << "zookeeper_init error!" << std::endl;
        exit(EXIT_FAILURE);
    }

    sem_t sem;                        // 创建一个信号量
    sem_init(&sem, 0, 0);             // 初始化为零
    zoo_set_context(m_zhandle, &sem); // 设置上下文，给指定的句柄添加一些额外的信息

    sem_wait(&sem); // 其实是等待全局的回调函数，的返回值
    std::cout << "zookeeper_init success!" << std::endl;
}

void ZkClient::Create(const char *path, const char *data, int datalen, int state)
{
    char path_buffer[128];
    int bufferlen = sizeof(path_buffer);
    int flag;
    // 先判断path表示的节点是否已经存在，如果存在就不再重复创建了
    flag = zoo_exists(m_zhandle, path, 0, nullptr);     
    if (ZNONODE == flag)    // 表示path的znode节点不存在
    {
        // 创建指定path的znode节点
        flag = zoo_create(m_zhandle, path, data, datalen,
                          &ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen);

        if (flag == ZOK)
        {
            std::cout << "znode create success... path:" << path << std::endl;
        }
        else
        {
            std::cout << "flag:" << flag << std::endl;
            std::cout << "znode create error... path:" << path << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

// 根据指定的path，获取znode节点的值
std::string ZkClient::GerData(const char *path)
{
    char buffer[64];
    int bufferlen = sizeof(buffer);
    int flag = zoo_get(m_zhandle, path, 0, buffer, &bufferlen, nullptr);
    if (flag != ZOK)
    {
        std::cout << "get znode error... path :" << path << std::endl;
        return "";
    }
    else
    {
        return buffer;
    }
}
