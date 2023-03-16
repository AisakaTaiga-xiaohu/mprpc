#include <iostream>
#include "mprpcapplication.h"
#include "user.pb.h"
#include "mprpcchannel.h"

int main(int argc, char **argv)
{
    // 整个程序启动之后，想使用mprpc框架来享受rpc服务，就一定需要先调用框架的初始化函数
    MprpcApplication::Init(argc, argv);

    // 演示调用远程发布的rpc方法Login
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
    
    // rpc方法的请求参数
    fixbug::LoginRequest request;
    request.set_name("cao danhua");
    request.set_pwd("123456");
    // rpc方法的响应
    fixbug::LoginResponse response;
    // 发起rpc方法的调用 同步的rpc调用过程 MprpcChannel::callmethod
    stub.Login(nullptr, &request, &response, nullptr);   // RpcChannel->RpcChannel::callMethod 集中来做所有rpc方法调用的参数序列化和网络发送
    // rpc调用完成，读取response结果
    if (response.result().errcode() == 0)
    {
        std::cout << "rpc login response success:" << response.sucess() << std::endl;
    }
    else 
    {
        std::cout << "rpc login response error : " << response.result().errmsg() << std::endl;
    }

    // 演示调用远程发布的rpc方法Register
    fixbug::RegisterRequest req;
    req.set_id(2000);
    req.set_name("mprpc");
    req.set_pwd("666666");
    fixbug::RegisterResponse res;
    stub.Register(nullptr, &req, &res, nullptr);
    if (res.result().errcode() == 0)
    {
        std::cout << "rpc register response success:" << res.sucess() << std::endl;
    }
    else 
    {
        std::cout << "rpc register response error : " << res.result().errmsg() << std::endl;
    }

    return 0;
}