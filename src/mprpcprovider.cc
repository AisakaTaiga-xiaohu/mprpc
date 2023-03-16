#include "mprpcprovider.h"
#include "mprpcapplication.h"
#include "rpcheader.pb.h"
#include "logger.h"
#include "zookeeperutil.h"

/*
    service_name => service描述
                            =》 service* 记录的服务对象
                            method_name => method方法对象
json protobuf
*/
// 这是框架提供给外部使用的，可以发布rpc方法的函数接口
void RpcProvider::NotifyService(google::protobuf::Service *service)
{
    /*
        出现问题：imcomplete class type
        原因：没有包含相应的累的头文件
    */
    ServiceInfo service_info;

    // 服务对象的描述信息
    const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();
    // 获取服务的名字
    std::string service_name = pserviceDesc->name();
    // 获取服务对象service的方法数量
    int methodCnt = pserviceDesc->method_count();

    // std::cout << "service_name : " << service_name << std::endl;
    LOG_INFO("service_name:%s", service_name.c_str());

    for (int i = 0; i < methodCnt; ++i)
    {
        // 获取服务对象指定下标的服务方法的描述(抽象类的描述)
        const google::protobuf::MethodDescriptor *pmethodDesc = pserviceDesc->method(i);
        std::string method_name = pmethodDesc->name();
        service_info.m_methodMap.insert({method_name, pmethodDesc});
        LOG_INFO("method_name:%s", method_name.c_str());
    }
    service_info.m_service = service;
    m_serviceMap.insert({service_name, service_info});
}

// 启动rpc服务节点，开始提供rpc服务
void RpcProvider::Run()
{
    // 组合了TcpServer--只有Run函数访问，尽量不要扩展访问范围
    // std::unique_ptr<muduo::net::TcpServer> m_tecserverPtr;
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    muduo::net::InetAddress address(ip, port);

    // 创建TcpServer对象
    muduo::net::TcpServer server(&m_eventLoop, address, "RpcProvider");
    // 绑定连接回调和消息读写回调方法
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection, this, std::placeholders::_1));
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage, this, std::placeholders::_1,
                                        std::placeholders::_2, std::placeholders::_3));

    // 设置muduo库的线程数量
    server.setThreadNum(4);

    // 把当前rpc节点上要发布的服务全部注册到zk上面，让rpc client可以从zk上发布服务
    // session timeout 30s zkclient 网络I/O 1/3 * timeout 时间发送心跳信息
    ZkClient zkCli;
    zkCli.Start();
    // service_name 为永久性节点 method_name为临时性节点
    for (auto &sp : m_serviceMap)
    {
        // service_name /UserServiceRpc
        std::string service_path = "/" + sp.first;
        zkCli.Create(service_path.c_str(), nullptr, 0);
        for (auto &mp : sp.second.m_methodMap)
        {
            // service_name/method_name /UserServiceRpc/Login 存储当前这个rpc服务节点主机的ip和port
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);
            // ZOO_EPHEMERAL表示znode是一个临时性节点
            zkCli.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
        }
    }

    // rpc服务端左闭启动，打印信息
    LOG_INFO("RpcProvider start service at ip:%s port:%d", ip.c_str(), port);

    // 启动网络服务
    server.start();
    // 以阻塞的方式等待远程连接请求
    m_eventLoop.loop();
}

// 新的socket连接回调
void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        // rpc client的连接断开了
        conn->shutdown();
    }
}

/*
在框架内部， RpcProvider和RpcConsumer协商好之间通信用的protobuf数据类型
service_name method_name args   定义proto的message类型，进行数据头的序列化和反序列化
                                service_name method_name args_size
16UserServiceLoginzhang san123456

header_size(4个子节) + header_str + args_str
10 "10"
10000 "10000" 这都是二进制四个子节，但是转换成string之后不符合要求
std::string insert和copy方法
*/
// 已建立连接用户的读写事件回调,如果远程有一个rpc服务的调用请求，那么OnMessage方法就会响应
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr &conn,
                            muduo::net::Buffer *buffer,
                            muduo::Timestamp)
{
    // 网络上接收的远程rpc调用请求的字符流 Login args
    std::string recv_buf = buffer->retrieveAllAsString();

    // 从字符流中读取前四个子节的内容
    uint32_t header_size = 0;
    recv_buf.copy((char *)&header_size, 4, 0);

    // 根据header_size读取数据头的原始字符流, 反序列化数据，得到rpc请求的详细信息
    std::string rpc_header_str = recv_buf.substr(4, header_size);
    mprpc::RpcHeader rpcHeader;
    std::string service_name;
    std::string method_name;
    uint32_t args_size;
    if (rpcHeader.ParseFromString(rpc_header_str))
    {
        // 数据头反序列化成功
        service_name = rpcHeader.service_name();
        method_name = rpcHeader.method_name();
        args_size = rpcHeader.args_size();
    }
    else
    {
        // 数据头反序列化失败
        // std::cout << "rpc_header_str" << rpc_header_str << " parse error!" << std::endl;
        LOG_ERROR("rpc_header_str:%s parse error!", rpc_header_str.c_str());
        return;
    }
    // 获取rpc方法参数的字符流数据
    std::string args_str = recv_buf.substr(4 + header_size, args_size);

    // 打印调试信息
    LOG_INFO("====================================================");
    LOG_INFO("header_size : %d", header_size);
    LOG_INFO("rpc_header_str : %s", rpc_header_str.c_str());
    LOG_INFO("service_name : %s", service_name.c_str());
    LOG_INFO("method_name : %s", method_name.c_str());
    LOG_INFO("args_size : %d", header_size);
    LOG_INFO("args_str : %s", args_str.c_str());
    LOG_INFO("====================================================");

    // 获取service对象和method对象
    auto it = m_serviceMap.find(service_name);
    if (it == m_serviceMap.end())
    {
        std::cout << service_name << "is not exist ! " << std::endl;
        return;
    }

    auto mit = it->second.m_methodMap.find(method_name);
    if (mit == it->second.m_methodMap.end())
    {
        std::cout << service_name << ":" << method_name << "is not exist ! " << std::endl;
    }

    google::protobuf::Service *service = it->second.m_service;      // 获取service对象 UserService
    const google::protobuf::MethodDescriptor *method = mit->second; // 获取method对象   Login

    // 生成rpc 方法调用的请求request和响应response参数
    google::protobuf::Message *resquest = service->GetRequestPrototype(method).New();
    if (!resquest->ParseFromString(args_str))
    {
        // std::cout << "resquest parse error:" << args_str << std::endl;
        LOG_ERROR("resquest parse error:%s", args_str.c_str());
        return;
    }
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();

    // 给下面的method方法的调用， 绑定一个Closure的回调函数  ---- 也可以使用lambda表达式绑定参数
    google::protobuf::Closure *done = google::protobuf::NewCallback<RpcProvider,
                                      const muduo::net::TcpConnectionPtr &,
                                      google::protobuf::Message *>
                                      (this, &RpcProvider::SendRpcResponse, conn, response);

    // 在框架上根据远程rpc请求，调用当前rpc节点上发布的方法
    // new UserService().Login(controller, resquest, response, done)
    service->CallMethod(method, nullptr, resquest, response, done);
}

// Closure回调操作，用于序列化Rpc的响应和网络发送
void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr &conn,
                                  google::protobuf::Message *response)
{
    std::string response_str;
    if (response->SerializeToString(&response_str))  // response进行序列化
    {
        // 序列化成功后，通过网络吧rpc方法执行的结果发送给rpc的调用方
        conn->send(response_str);
    }
    else
    {
        LOG_ERROR("serialize response_str error!");
        // std::cout << "serialize response_str error!" << std::endl;    
    }
    conn->shutdown();  // 模拟http的短链接服务，由rpcprovider主动断开连接
}