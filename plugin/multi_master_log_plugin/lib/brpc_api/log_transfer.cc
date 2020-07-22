/*
 * @Author: wei
 * @Date: 2020-07-09 15:49:19
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-07-16 10:52:38
 * @Description: file content
 * @FilePath: /multi_master_log_plugin/lib/brpc_api/log_transfer.cc
 */

#include "log_transfer.h"

#define TRANSFER_DEBUG_HEADER " [@LogTransfer DEBUG] : "
#define TRANSFER_ERROR_HEADER " [@LogTransfer ERROR] : "

//LogTransfer global_log_transfer;

void OnLogSendRPCDone(MMLP_BRPC::LogSendResponse * response, brpc::Controller* cntl,LogTransfer * transfer)
{

    if (cntl->Failed()) {
        if(DEBUG_LOG_SEND_TIME != 0)
        {
            log_send_async_rpc_failed_count++;
            log_send_async_rpc_failed_time += cntl->latency_us();
        }
        std::cout << TRANSFER_ERROR_HEADER <<" Fail to send Log Send Request, " << cntl->ErrorText() << std::endl;
        return;
    }
    else
    {
        if(DEBUG_LOG_SEND_TIME != 0)
        {
            log_send_async_rpc_count++;
            log_send_async_rpc_time += cntl->latency_us();
        }
#if BRPC_HANDLE_DEBUG
        std::cout << TRANSFER_DEBUG_HEADER << " Success to send Log Send Request :" << cntl->latency_us() << std::endl;
#endif
    }

    transfer->return_to_send_pool(cntl,response);
}


/**
 * ============ TrxLogService_impl
 * */

void TrxLogService_impl::sendLog(::google::protobuf::RpcController* controller,
           const ::MMLP_BRPC::LogSendRequest* request,
           ::MMLP_BRPC::LogSendResponse* response,
           ::google::protobuf::Closure* done)
{
    brpc::ClosureGuard done_guard(done);
    //brpc::Controller* cntl = static_cast<brpc::Controller*>(controller);

    //TODO: 函数对象或是函数指针完成一些处理

    response->set_send_reply(1); // recive success
}

void TrxLogService_impl::requireLog(::google::protobuf::RpcController* controller,
           const ::MMLP_BRPC::LogRequireRequest* request,
           ::MMLP_BRPC::LogRequireResponse* response,
           ::google::protobuf::Closure* done)
{

}

/**
 * ============ LogTransfer
 * */

// void LogTransfer::async_arg_init(uint32_t & pool_size)
// {
//     AsyncArg tmp_arg;
//     tmp_arg.response_ptr = NULL;
//     tmp_arg.cntrl_ptr = NULL;
//     for(int i = 0; i < pool_size ; i++)
//     {
//         //send arg
//         tmp_arg.cntrl_ptr = new brpc::Controller;
//         tmp_arg.response_ptr = new ::MMLP_BRPC::LogSendResponse;
//         async_send_pool.push_back(tmp_arg);
//         //require arg
//         tmp_arg.cntrl_ptr = new brpc::Controller;
//         tmp_arg.response_ptr = new ::MMLP_BRPC::LogRequireResponse;
//         async_require_pool.push_back(tmp_arg);
//     }

//     //TODO:初始化异步参数pool
// }

// int LogTransfer::move_send_from_pool(MMLP_BRPC::LogSendResponse * & res , brpc::Controller * & cntrl)
// {
//     //TODO:条件变量控制线程等待pool中有数据
//     return 1;
// }

// int LogTransfer::move_require_from_pool(MMLP_BRPC::LogRequireResponse * & res, brpc::Controller * & cntrl)
// {
//     return 1;
// }

// int LogTransfer::move_from_using(TransferType type, brpc::Controller * cntrl)
// {
//     //TODO:通知条件变量pool中增加了数据
//     return 1;
// }
LogTransfer::~LogTransfer()
{
    for(auto it = connection_map.begin() ; it != connection_map.end() ; it++)
    {
        delete it->second;
    }
    connection_map.clear();

    while(!async_send_pool.empty())
    {
        delete async_send_pool.front().cntrl_ptr;
        delete async_send_pool.front().response_ptr;
        async_send_pool.pop();
    }

    while (!async_require_pool.empty())
    {
        delete async_require_pool.front().cntrl_ptr;
        delete async_require_pool.front().response_ptr;
        async_require_pool.pop();
    }

}


int LogTransfer::get_send_async_arg(MMLP_BRPC::LogSendResponse * & res,brpc::Controller * & cntrl)
{
    if(async_send_pool.empty())
    {
        cntrl = new brpc::Controller;
        res =  new MMLP_BRPC::LogSendResponse;
    }
    else
    {
        async_send_pool_mutex.lock();
        res = async_send_pool.front().response_ptr;
        cntrl = async_send_pool.front().cntrl_ptr;
        async_send_pool.pop();
        async_send_pool_mutex.unlock();
    }
    return 1;
}

int LogTransfer::return_to_send_pool(brpc::Controller * cntrl,MMLP_BRPC::LogSendResponse * res)
{
    cntrl->Reset();
    res->Clear();

    SendAsyncArg tmp_arg;
    tmp_arg.response_ptr = res;
    tmp_arg.cntrl_ptr = cntrl;
    async_send_pool_mutex.lock();
    async_send_pool.push(tmp_arg);
    async_send_pool_mutex.unlock();
    return 1;
}

int LogTransfer::addr_init()
{
     //local_str
    std::string local_str = brpc_local_node_ptr;

    std::stringstream local_ss(local_str);
    std::string port = "";
    getline(local_ss,local_ip,':');
    getline(local_ss,port);
    local_port = stoi(port);

    return 1;
}

int LogTransfer::client_init()
{
    std::string peers_str = brpc_peer_nodes_ptr;

    //channel option
    brpc::ChannelOptions option;
    option.protocol = brpc::PROTOCOL_BAIDU_STD;
    option.connection_type = brpc::CONNECTION_TYPE_SINGLE;
    // option.timeout_ms = 1000/*milliseconds*/;
    // option.max_retry = 5;

    //peers_str
    std::stringstream peers_ss(peers_str);
    std::string peer_addr = "";

    brpc::Channel * new_channel;
    while(!peers_ss.eof())
    {
        getline(peers_ss,peer_addr,',');
#if BRPC_HANDLE_DEBUG
        std::cout << TRANSFER_DEBUG_HEADER <<" one peer : " << peer_addr << std::endl;
#endif
#if !BRPC_HANDLE_DEBUG
        if(peer_addr != brpc_local_node_ptr)
        {
#endif
            new_channel = new ::brpc::Channel;
            connection_map[peer_addr] = new_channel;
            if(new_channel->Init(peer_addr.c_str(),&option) != 0)
            {
                std::cout << TRANSFER_ERROR_HEADER << peer_addr << " - connect failed" <<  std::endl;
                //return -1;
            }
            send_channels.AddChannel(new_channel,::brpc::DOESNT_OWN_CHANNEL,NULL,NULL);
            require_channels.AddChannel(new_channel,::brpc::DOESNT_OWN_CHANNEL,NULL,NULL);
#if !BRPC_HANDLE_DEBUG
        }
#endif
    }
    return 1;
}

int LogTransfer::server_init()
{
    if(server.AddService(&trxlog_service,brpc::SERVER_OWNS_SERVICE) != 0)
    {
        std::cout << "Add Service Failed" << std::endl;
        return -1;
    }
#if BRPC_HANDLE_DEBUG
    std::cout <<"Server IP :" << local_ip <<" : " << local_port << std::endl;
#endif
    if (server.Start(local_port, NULL) != 0) {
        std::cout << "Fail to start TrxLogServer" << std::endl;
        return -1;
    }

    return 1;
}

int LogTransfer::init()
{
    int ret = 1;
    ret = addr_init();
    ret = server_init();
    ret = client_init();
    return ret;
}

int LogTransfer::sync_send_log(TrxID id,bool valid,std::string & msg,uint64_t * latency_ptr)
{
    ::brpc::Controller cntrl;
    ::MMLP_BRPC::LogSendRequest request;
    ::MMLP_BRPC::LogSendResponse response;

    request.set_trxid(id);
    request.set_trxlogmsg(msg);
    request.set_is_valid(valid);

    ::MMLP_BRPC::TrxLogService_Stub stub(&send_channels);
    stub.sendLog(&cntrl,&request,&response,NULL);

    if(!cntrl.Failed())
    {
        //latency record
        if(latency_ptr != NULL)
        {
            *latency_ptr = cntrl.latency_us();
        }
#if BRPC_HANDLE_DEBUG
        std::cout << TRANSFER_DEBUG_HEADER << "SYNC SEND LOG SUCCESS " << std::endl;
#endif
        return 1;
    }
    else
    {
        if(latency_ptr != NULL)
        {
            *latency_ptr = cntrl.latency_us();
        }
        std::cout << TRANSFER_ERROR_HEADER << " SYNC SEND LOG ERROR ! " << cntrl.ErrorText() << std::endl;
        return -1;
    }
}

int LogTransfer::async_send_log(TrxID id , bool valid,std::string & msg,uint64_t * latency_ptr)
{
    ::MMLP_BRPC::LogSendRequest request;
    request.set_trxid(id);
    request.set_trxlogmsg(msg);
    request.set_is_valid(valid);

    brpc::Controller * cntrl_ptr = NULL;
    ::MMLP_BRPC::LogSendResponse * response_ptr = NULL;
    get_send_async_arg(response_ptr,cntrl_ptr);
    ::MMLP_BRPC::TrxLogService_Stub stub(&send_channels);
    stub.sendLog(cntrl_ptr,&request,response_ptr,brpc::NewCallback(OnLogSendRPCDone,response_ptr,cntrl_ptr,this));

    if(!cntrl_ptr->Failed())
    {
        //latency record
        if(latency_ptr != NULL)
        {
            *latency_ptr = cntrl_ptr->latency_us();
        }
#if BRPC_HANDLE_DEBUG
        std::cout << TRANSFER_DEBUG_HEADER << "ASYNC(not done) SYNC SEND LOG SUCCESS " << std::endl;
#endif
        return 1;
    }
    else
    {
        if(latency_ptr != NULL)
        {
            *latency_ptr = cntrl_ptr->latency_us();
        }
        std::cout << TRANSFER_ERROR_HEADER << " SYNC SEND LOG ERROR ! " << cntrl_ptr->ErrorText() << std::endl;
        return -1;
    }

    return 1;
}