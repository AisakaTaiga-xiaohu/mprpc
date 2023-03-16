#include <iostream>
#include "mprpcapplication.h"
#include "friend.pb.h"

int main(int argc, char **argv)
{
    MprpcApplication::Init(argc, argv);

    fixbug::FriendServiceRpc_Stub stub(new MprpcChannel());

    fixbug::GetFriendListRequest req;
    req.set_userid(1000);

    fixbug::GetFriendListResponse res;

    MprpcController controller;
    stub.GetFriendList(&controller, &req, &res, nullptr);

    std::cout << "print result :" << std::endl;
    if (controller.Failed())
    {
        std::cout << controller.ErrorText() << std::endl;
    }
    else
    {
        if (res.result().errcode() == 0) 
        {
            std::cout << "rpc GetFriend response Success !" << std::endl;
            for (int i = 0; i < res.friends_size(); ++i) 
            {
                std::cout << "index:" << (i + 1) << " name :" << res.friends(i) << std::endl;
            }
        }
        else 
        {
            std::cout << "rpc GetFriend response Error ! error : " << res.result().errmsg() << std::endl;
        }
    }

    return 0;
}