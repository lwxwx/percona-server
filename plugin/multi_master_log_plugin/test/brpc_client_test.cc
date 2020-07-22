/*
 * @Author: wei
 * @Date: 2020-07-07 16:39:47
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-07-08 16:58:20
 * @Description: file content
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/test/brpc_client_test.cc
 */

#include "test.pb.h"
#include <brpc/channel.h>
#include <iostream>

using namespace std;

int main(void)
{
       // A Channel represents a communication line to a Server. Notice that
    // Channel is thread-safe and can be shared by all threads in your program.
    brpc::Channel channel;

    // Initialize the channel, NULL means using default options.
    brpc::ChannelOptions options;
    options.protocol = brpc::PROTOCOL_BAIDU_STD;
    options.connection_type = brpc::CONNECTION_TYPE_SINGLE;
    options.timeout_ms = 1000/*milliseconds*/;
    options.max_retry = 5;
    if (channel.Init("127.0.0.1:50115" , &options) != 0) {
        LOG(ERROR) << "Fail to initialize channel";
        return -1;
    }

    // Normally, you should not call a Channel directly, but instead construct
    // a stub Service wrapping it. stub can be shared by all threads as well.
    test::TestService_Stub stub(&channel);

    // Send a request and wait for the response every 1 second.
    int log_id = 0;
    while (!brpc::IsAskedToQuit()) {
        // We will receive response synchronously, safe to put variables
        // on stack.
        test::TestRequest request;
        test::TestResponse response;
        brpc::Controller cntl;

        request.set_message("hello world");

        cntl.set_log_id(log_id ++);  // set by user


        stub.Test(&cntl, &request, &response, NULL);
        if (!cntl.Failed()) {
            cout << "Test Client Success" << endl;
        } else {
            cout << "Test Client Failed" << endl;
            //LOG(WARNING) << cntl.ErrorText();
        }
        usleep(1000 * 1000L);
    }

    cout << "Client is going to quit" << endl;

    return 0;
}