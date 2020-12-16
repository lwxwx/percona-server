
/*
 * @Author: liu
 * @Date: 2020-10-22 10:00
 * @LastEditors: Do not edit
 * @LastEditTime: 
 * @Description: file content
 * @FilePath: /multi-master-tool/lock_table/client.cpp
 */

#include "locktable_client.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <cstdint>
#include "debug.h"
#include <sys/time.h>

//for debug
unsigned long long latency_us=0;
struct timeval tv;

InformReceiver *plugin_remote_locktable_receiver_ptr = NULL;
RemoteLockTable *plugin_remote_locktable_sender_ptr = NULL;

//log configure
auto my_logger = spdlog::basic_logger_mt("REMOTE LOCKTABLE", "./locktable.log");

//condition variable for thread communication
std::map<std::string ,std::pair<std::mutex ,std::condition_variable>> shared_cv;

/**
***RemoteLockTable
***The client send lock request to the server
***/

int RemoteLockTable::init()
{
    #if LOCKTABLE_DEBUG
    my_logger->set_level(spdlog::level::debug);
    #endif
    my_logger->flush_on(spdlog::level::debug);
    brpc::ChannelOptions options;
    options.protocol = brpc::PROTOCOL_BAIDU_STD;
    options.connection_type = brpc::CONNECTION_TYPE_SINGLE;
    options.timeout_ms=-1;
    options.connect_timeout_ms=-1;
    if (channel.Init(server_addr.c_str(), &options) != 0) {
        my_logger->error("[lock request sender] Fail to initialize channel");
        return -1;
    }
    my_logger->info("[lock request sender] initialize");
}

int RemoteLockTable::send_request(TableID table_id,PageID page_id,std::string request_type,int c_s_port)
{    
    stub = new lock_table::LOCKService_Stub(&channel);
    lock_table::LOCKRequest request;
    lock_table::LOCKResponse response;
    brpc::Controller cntl ;

    std::string message=request_type+","+std::to_string(table_id)+","+std::to_string(page_id)+","+std::to_string(c_s_port);
    request.set_message(message);

    //syn
    stub->LOCKTable(&cntl, &request, &response, NULL);
    if(cntl.Failed())
    {
        my_logger->error("[lock request sender error] "+cntl.ErrorText());
    }

    if(response.message()=="wait"){
        std::string req_type=(request_type=="write")?"2":"1";
        my_logger->info("[lock request sender] wait for lock "+std::to_string(table_id)+"-"+std::to_string(page_id)+","+request_type+" request = "+message);
        std::unique_lock<std::mutex> locker(shared_cv[std::to_string(table_id)+"-"+std::to_string(page_id)+","+req_type].first);
        //wait lock 
        shared_cv[std::to_string(table_id)+"-"+std::to_string(page_id)+","+req_type].second.wait(locker);
        locker.unlock();
    }
}

/**
***InformReceiver
***The client is notified that it has acquired the lock and does not need to block any more
***/

void LockInformImpl::LockInform(google::protobuf::RpcController* cntl_base, const lock_inform::InformRequest* request, lock_inform::InformResponse* response, google::protobuf::Closure* done) 
{
    brpc::ClosureGuard done_guard(done);
    brpc::Controller* cntl = static_cast<brpc::Controller*>(cntl_base);
    std::lock_guard<std::mutex> guard(lock);
        
    // time_t now = time(0);
    // char* dt = ctime(&now);
        
    // std::cout<<"Received lock information from " << cntl->remote_side() << " to " << cntl->local_side()<< "-- message:" << request->message()<<" ,time:"<<dt<<std::endl;

    std::string recive_mess=request->message();
    size_t pos=recive_mess.find(",");std::string talk_client =recive_mess.substr(0,pos);
    std::string table_page_tp=recive_mess.substr(pos+1,recive_mess.size());
     my_logger->info("[lock info receiver] can get lock "+request->message());

    std::unique_lock<std::mutex> locker(shared_cv[request->message()].first);
    locker.unlock();
    shared_cv[request->message()].second.notify_all();   
    response->set_message("The client thread receives the lock !");
}

int InformReceiver::init()
{
    #if LOCKTABLE_DEBUG
    my_logger->set_level(spdlog::level::debug);
    #endif
    my_logger->flush_on(spdlog::level::debug);

    if (server.AddService(&inform_server, brpc::SERVER_OWNS_SERVICE) != 0) {
        my_logger->error("[lock info receiver] Fail to add inform receiver in client");
        return -1;
    }

    brpc::ServerOptions options;
    options.idle_timeout_sec = -1;
    if (server.Start(port, &options) != 0) {
        my_logger->error("[lock info receiver] Fail to start inform receiver in client");
        return -1;
    }
    my_logger->info("[lock info receiver] inform receiver start in client");
}

int InformReceiver::run()
{
    my_logger->info("[lock info receiver] client's inform receiver is run");
    server.RunUntilAskedToQuit();
}