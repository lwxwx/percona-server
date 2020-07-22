/*
 * @Author: wei
 * @Date: 2020-07-07 16:24:29
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-07-16 10:49:59
 * @Description: file content
 * @FilePath: /multi_master_log_plugin/lib/brpc_api/log_transfer.h
 */

#ifndef LOG_TRANSFER_HEADER
#define LOG_TRANSFER_HEADER

#include "mmlp_type.h"
#include "trx_log.pb.h"
#include <brpc/channel.h>
#include <brpc/server.h>
#include <brpc/parallel_channel.h>
#include <map>
#include <sstream>
#include <string>
#include <queue>
#include <mutex>

#define BRPC_HANDLE_DEBUG 0

// #define DEFAULT_ARG_POOL_SIZE 50;

class TrxLogService_impl : public MMLP_BRPC::TrxLogService
{
    public:
    virtual void sendLog(::google::protobuf::RpcController* controller,
                       const ::MMLP_BRPC::LogSendRequest* request,
                       ::MMLP_BRPC::LogSendResponse* response,
                       ::google::protobuf::Closure* done);

    virtual void requireLog(::google::protobuf::RpcController* controller,
                       const ::MMLP_BRPC::LogRequireRequest* request,
                       ::MMLP_BRPC::LogRequireResponse* response,
                       ::google::protobuf::Closure* done);
};

struct SendAsyncArg
{
    MMLP_BRPC::LogSendResponse * response_ptr;
    brpc::Controller * cntrl_ptr;
};

struct RequireAsyncArg
{
    MMLP_BRPC::LogRequireResponse * response_ptr;
    brpc::Controller * cntrl_ptr;
};

enum TransferType
{
    SEND_BROADCAST,
    REQUIRE_BROADCAST
};

class LogTransfer
{
    private:
    brpc::Server server;
    int local_port;
    std::string local_ip;

    TrxLogService_impl trxlog_service;
    std::map<std::string,brpc::Channel*> connection_map;

    brpc::ParallelChannel send_channels;
    brpc::ParallelChannel require_channels;

    std::queue<SendAsyncArg> async_send_pool;  //which can use again
    std::queue<RequireAsyncArg> async_require_pool;
    std::mutex async_send_pool_mutex;
    std::mutex async_require_pool_mutex;

    int addr_init();
    int client_init(); // brpc client init
    int server_init(); // brpc server init

    public:
    ~LogTransfer();
    int init();
    int async_send_log(TrxID id , bool valid, std::string & msg,uint64_t * latency_ptr);
    int sync_send_log(TrxID id,bool valid,std::string & msg,uint64_t * latency_ptr);
    int async_require_log(TrxID id, uint64_t * latency_ptr);

    int get_send_async_arg(MMLP_BRPC::LogSendResponse * & res,brpc::Controller * & cntrl);
    int get_require_async_arg(MMLP_BRPC::LogRequireResponse * & res,brpc::Controller * & cntrl){return 1;};
    int return_to_send_pool(brpc::Controller * cntrl,MMLP_BRPC::LogSendResponse * res);
    int return_to_require_pool(brpc::Controller * cntrl,MMLP_BRPC::LogRequireResponse * res){return 1;};
};

// extern LogTransfer global_log_transfer;

void OnLogSendRPCDone(MMLP_BRPC::LogSendResponse * response, brpc::Controller* cntl,LogTransfer * transfer);

#endif

