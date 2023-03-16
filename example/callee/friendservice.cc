#include <iostream>
#include <string>
#include <vector>
#include "logger.h"
#include "friend.pb.h"
#include "mprpcapplication.h"
#include "mprpcprovider.h"

class FriendService : public fixbug::FriendServiceRpc
{
public:
    std::vector<std::string> GetFriendList(uint32_t userid)
    {
        std::cout << "do GetFriendList service! userid :" << userid << std::endl;
        std::vector<std::string> vec;
        vec.push_back("li longkang");
        vec.push_back("feng rongmin");
        vec.push_back("xu jianhang");

        return vec;
    }

    void GetFriendList(::google::protobuf::RpcController *controller,
                       const ::fixbug::GetFriendListRequest *request,
                       ::fixbug::GetFriendListResponse *response,
                       ::google::protobuf::Closure *done)
    {
        uint32_t id = request->userid();

        std::vector<std::string> friendList = GetFriendList(id);

        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        for (auto& friendnum : friendList) 
        {
            std::string *p = response->add_friends();
            *p = friendnum;
        }

        done->Run();
    }
};

int main(int argc, char **argv)
{

    MprpcApplication::Init(argc, argv);

    RpcProvider provider;
    provider.NotifyService(new FriendService());

    provider.Run();

    return 0;
}