/*
 * @Author: wei
 * @Date: 2020-07-07 16:24:29
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-07-28 15:58:06
 * @Description: file content
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/lib/brpc_api/log_transfer.h
 */

#ifndef LOG_TRANSFER_HEADER
#define LOG_TRANSFER_HEADER

//#include "trx_info.h"
#include "trx_log.h"
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

class MessageHandle
{
    public:
    virtual int handle(void * request,void * response) =0;
};

class TrxLogService_impl : public MMLP_BRPC::TrxLogService
{
    private:

    MessageHandle * send_service_message_handle_ptr;
    MessageHandle * require_service_message_handle_ptr;

    public:

    void init(MessageHandle * send_service_handle,MessageHandle * require_service_handle)
    {
        send_service_message_handle_ptr = send_service_handle;
        require_service_message_handle_ptr = require_service_handle;
    }

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
    std::map<std::string,brpc::Channel*> connection_failed_map;
    brpc::ChannelOptions basic_options;

    brpc::ParallelChannel send_channels;
    brpc::ParallelChannel require_channels;

    std::queue<SendAsyncArg> async_send_pool;  //which can use again
    std::queue<RequireAsyncArg> async_require_pool;
    std::mutex async_send_pool_mutex;
    std::mutex async_require_pool_mutex;

    int addr_init();
    int client_init(); // brpc client init
    int server_init(MessageHandle * send_service_handle,MessageHandle * require_service_handle); // brpc server init

    int check_failed_connections_and_retry();

    public:
    ~LogTransfer();
    // int set_arg(TransferInitArg key,void * val);
    int init(MessageHandle * send_service_handle,MessageHandle * require_service_handle);
   
	//log send functions
	int async_send_log(TrxLog & log,uint64_t * latency_ptr);
    int sync_send_log(TrxLog & log,uint64_t * latency_ptr);

	//log require function
    int async_require_log(TrxID trx_id, uint64_t * latency_ptr,MessageHandle * handle_prt);
	
	//send or require arg pool
    int get_send_async_arg(MMLP_BRPC::LogSendResponse * & res,brpc::Controller * & cntrl);
    int return_to_send_pool(brpc::Controller * cntrl,MMLP_BRPC::LogSendResponse * res);
	int get_require_async_arg(MMLP_BRPC::LogRequireResponse * & res,brpc::Controller * & cntrl);
	int return_to_require_pool(brpc::Controller * cntrl,MMLP_BRPC::LogRequireResponse * res);
};

// extern LogTransfer global_log_transfer;

void OnLogSendRPCDone(MMLP_BRPC::LogSendResponse * response, brpc::Controller* cntl,LogTransfer * transfer);
void OnLogRequireRPCDone(MMLP_BRPC::LogRequireResponse * response, brpc::Controller* cntl,LogTransfer * transfer,MessageHandle * handle_prt);

// debug functions
void debug_print_SendRequest(const MMLP_BRPC::LogSendRequest & res);

#endif

