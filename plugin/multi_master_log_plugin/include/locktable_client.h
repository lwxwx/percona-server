/*
 * @Author: liu
 * @Date: 2020-10-22 10:00
 * @LastEditors: Do not edit
 * @LastEditTime: 
 * @Description: file content
 * @FilePath: /multi-master-tool/lock_table/client.h
 */
// #include <butil/time.h>
#include <brpc/channel.h>
#include <brpc/server.h>
#include "lock_table.pb.h"
#include "lock_inform.pb.h"
#include <iostream>
#include <ctime>
#include <string>
#include <random>
#include <mutex>
#include <stdlib.h>
#include <condition_variable>
#include <map>
#include <thread>
#include <chrono>
#include "mmlp_type.h"

#define LOCKTABLE_DEBUG 0


//condition variable for thread communication
extern std::map<std::string,std::pair<std::mutex,std::condition_variable>> shared_cv;

struct mess
{
    TableID table_id;
    PageID page_id;
    std::string request_type;
    int c_s_port;
    std::string tid;
};

// The client send lock request to the server
class RemoteLockTable
{
    private:
    mess send_message;
    std::string server_addr=remote_locktable_server_addr;
    brpc::Channel channel;
    lock_table::LOCKService_Stub * stub;

    public:        
    int init();
    int send_request(TableID table_id,PageID page_id,std::string request_type,int c_s_port);
};

//The client is notified that it has acquired the lock and does not need to block any more
class LockInformImpl : public lock_inform::InformService {
    private:
        std::mutex lock;

    public:
        LockInformImpl() {};
        virtual ~LockInformImpl() {};
        virtual void LockInform(google::protobuf::RpcController* cntl_base, const lock_inform::InformRequest* request, lock_inform::InformResponse* response, google::protobuf::Closure* done);
};

class InformReceiver
{
    private:
    int port=remote_locktable_local_port;
    brpc::Server server;
    LockInformImpl inform_server;

    public:
    int init();
    int run();
};

extern InformReceiver *plugin_remote_locktable_receiver_ptr;
extern RemoteLockTable *plugin_remote_locktable_sender_ptr;