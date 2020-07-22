/*
 * @Author: wei
 * @Date: 2020-07-07 16:40:03
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-07-08 16:58:11
 * @Description: file content
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/test/brpc_server_test.cc
 */

#include "test.pb.h"
#include <brpc/server.h>
#include <iostream>

using namespace std;

class MyTestService : public test::TestService
{
    public:
    virtual void Test(google::protobuf::RpcController * controller,const test::TestRequest* request,test::TestResponse* response,google::protobuf::Closure* done)
                       {
                            brpc::ClosureGuard done_guard(done);

                            brpc::Controller* cntrl = static_cast<brpc::Controller*>(controller);

                            cout <<  "Test:[" << cntrl->remote_side() << " From " << cntrl->local_side() << "] : " << request->message() << endl;

                            response->set_message("Call OK");



                       }
};

int main(void)
{
    brpc::Server server;
    MyTestService test_ser;

    if(server.AddService(&test_ser,brpc::SERVER_OWNS_SERVICE) != 0)
    {
        cout << "Add Service Failed" << endl;
        return -1;
    }

      // Start the server.
    brpc::ServerOptions options;
    options.idle_timeout_sec = 60;
    if (server.Start(50115, &options) != 0) {
        cout << "Fail to start EchoServer" << endl;
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();

    return 0;
}