/*
 * @Author: wei
 * @Date: 2020-07-09 15:49:19
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-07-28 16:09:26
 * @Description: file content
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/lib/brpc_api/log_transfer.cc
 */

#include "log_transfer.h"
#include <cstddef>
#include <iostream>
#include "brpc/callback.h"
#include "brpc/closure_guard.h"
#include "brpc/controller.h"
#include "mmlp_type.h"
//#include "trx_info.h"
#include "trx_log.pb.h"

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


void OnLogRequireRPCDone(MMLP_BRPC::LogRequireResponse * response, brpc::Controller* cntl,LogTransfer * transfer,MessageHandle * require_response_handle)
{
	if(cntl->Failed())
	{
		//failed count and time 
		if(DEBUG_LOG_REQUIRE_TIME != 0)
		{
			log_require_async_rpc_failed_count++;
			log_require_async_rpc_failed_time += cntl->latency_us();
		}
		std::cout << TRANSFER_ERROR_HEADER << "Fail to send Log Require Request, " << cntl->ErrorText() << std::endl;
		return;
	}
	else
	{
		require_response_handle->handle(NULL , response);
		// SUCCESS COUNT AND TIME
		if(DEBUG_LOG_REQUIRE_TIME != 0)
		{
			log_require_async_rpc_count++;
			log_require_async_rpc_time += cntl->latency_us();
		}
#if BRPC_HANDLE_DEBUG
		std::cout << TRANSFER_DEBUG_HEADER << "Success to send Log Require Request :" << cntl->latency_us() << std::endl;
#endif
	}
	transfer->return_to_require_pool(cntl, response);
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

#if BRPC_HANDLE_DEBUG
    debug_print_SendRequest(*request);
#endif

    int result = send_service_message_handle_ptr->handle((void*)request,(void*)response);

#if BRPC_HANDLE_DEBUG
    std::cout << TRANSFER_DEBUG_HEADER <<"Send Request Handle Result: " << result <<std::endl;
#endif

    response->set_send_reply(result); // recive success
}

void TrxLogService_impl::requireLog(::google::protobuf::RpcController* controller,
           const ::MMLP_BRPC::LogRequireRequest* request,
           ::MMLP_BRPC::LogRequireResponse* response,
           ::google::protobuf::Closure* done)
{
	brpc::ClosureGuard done_guard(done);

	int result = require_service_message_handle_ptr->handle((void*)request,(void*)response);

#if BRPC_HANDLE_DEBUG
	std::cout << TRANSFER_DEBUG_HEADER << "Require Request Handle Result: " << result << std::endl;	
#endif
	response->set_trxid(request->trxid());
	response->set_require_reply(result);
}

/**
 * ============ LogTransfer
 * */

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
    basic_options.protocol = brpc::PROTOCOL_BAIDU_STD;
    basic_options.connection_type = brpc::CONNECTION_TYPE_SINGLE;
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
//#if !BRPC_HANDLE_DEBUG
        if(peer_addr != brpc_local_node_ptr)
        {
//#endif
            new_channel = new ::brpc::Channel;
            connection_map[peer_addr] = new_channel;
            if(new_channel->Init(peer_addr.c_str(),&basic_options) != 0)
            {
                connection_failed_map[peer_addr] = new_channel;
                std::cout << TRANSFER_ERROR_HEADER << peer_addr << " - connect failed" <<  std::endl;
                //return -1;
            }
            send_channels.AddChannel(new_channel,::brpc::DOESNT_OWN_CHANNEL,NULL,NULL);
            require_channels.AddChannel(new_channel,::brpc::DOESNT_OWN_CHANNEL,NULL,NULL);
//#if !BRPC_HANDLE_DEBUG
        }
//#endif
    }
    return 1;
}

int LogTransfer::check_failed_connections_and_retry()
{
    if(connection_failed_map.empty())
    {
        return 0;
    }
    for(auto it  = connection_failed_map.begin() ; it != connection_failed_map.end(); it++)
    {
        if(it->second->Init(it->first.c_str(),&basic_options) !=0)
        {
            std::cout << TRANSFER_ERROR_HEADER << it->first << " - RETRY connect failed" <<  std::endl;
        }
        else
        {
            connection_failed_map.erase(it->first);
        }
    }
    if(!connection_failed_map.empty())
    {
        return -1;
    }
    return 1;
}

int LogTransfer::server_init(MessageHandle * send_service_handle,MessageHandle * require_service_handle)
{
    trxlog_service.init(send_service_handle,require_service_handle);
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

int LogTransfer::init(MessageHandle * send_service_handle,MessageHandle * require_service_handle)
{
    int ret = 1;
    ret = addr_init();
    ret = server_init(send_service_handle,require_service_handle);
    ret = client_init();
    return ret;
}

int LogTransfer::sync_send_log(TrxLog & trxlog,uint64_t * latency_ptr)
{
    if(check_failed_connections_and_retry() < 0)
    {
        std::cout << TRANSFER_ERROR_HEADER << " SYNC SEND LOG ERROR ! " << "Because failed connections" << std::endl;
        return -1;
    }

    ::brpc::Controller cntrl;
    ::MMLP_BRPC::LogSendRequest request;
    ::MMLP_BRPC::LogSendResponse response;

    request.set_trxid(trxlog.get_trxID());
    //request.set_trxlogmsg(msg);
    request.set_is_valid(!trxlog.get_rollback_status());
    trxlog.trx_log_encode_into_msg(request);

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

int LogTransfer::async_send_log(TrxLog & trxlog,uint64_t * latency_ptr)
{
    if(check_failed_connections_and_retry() < 0)
    {
        std::cout << TRANSFER_ERROR_HEADER << " ASYNC SEND LOG ERROR ! " << "Because failed connections" << std::endl;
        return -1;
    }

    ::MMLP_BRPC::LogSendRequest request;
    request.set_trxid(trxlog.get_trxID());
   // request.set_trxlogmsg(msg);
    request.set_is_valid(!trxlog.get_rollback_status());
    trxlog.trx_log_encode_into_msg(request);

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
        std::cout << TRANSFER_ERROR_HEADER << " ASYNC SEND LOG ERROR ! " << cntrl_ptr->ErrorText() << std::endl;
        return -1;
    }

    return 1;
}

int LogTransfer::get_require_async_arg(MMLP_BRPC::LogRequireResponse *&res, brpc::Controller *&cntrl)
{
	if(async_require_pool.empty())
	{
		cntrl = new brpc::Controller;
		res = new MMLP_BRPC::LogRequireResponse;
	}
	else
	{
		async_require_pool_mutex.lock();
		res = async_require_pool.front().response_ptr;
		cntrl = async_require_pool.front().cntrl_ptr;
		async_require_pool.pop();
		async_require_pool_mutex.unlock();
	}
	return 1;
}

int LogTransfer::return_to_require_pool(brpc::Controller *cntrl, MMLP_BRPC::LogRequireResponse *res)
{
	cntrl->Reset();
	res->Clear();

	RequireAsyncArg tmp_arg;
	tmp_arg.response_ptr =  res;
	tmp_arg.cntrl_ptr = cntrl;
	async_require_pool_mutex.lock();
	async_require_pool.push(tmp_arg);
	async_require_pool_mutex.unlock();
	return 1;
}

int LogTransfer::async_require_log(TrxID trx_id, uint64_t *latency_ptr,MessageHandle * require_response_handle)
{
    if(check_failed_connections_and_retry() < 0)
    {
        std::cout << TRANSFER_ERROR_HEADER << " ASYNC SEND LOG ERROR ! " << "Because failed connections" << std::endl;
        return -1;
    }

	::MMLP_BRPC::LogRequireRequest request;
	request.set_trxid(trx_id);
	
	brpc::Controller * cntrl_ptr = NULL;
	::MMLP_BRPC::LogRequireResponse * response_ptr = NULL;
	get_require_async_arg(response_ptr, cntrl_ptr);
	::MMLP_BRPC::TrxLogService_Stub stub(&require_channels);
	stub.requireLog(cntrl_ptr,&request,response_ptr,brpc::NewCallback(OnLogRequireRPCDone,response_ptr,cntrl_ptr,this,require_response_handle));

	if(!cntrl_ptr->Failed())
	{
		//latency record
		if(latency_ptr != NULL)
		{
			*latency_ptr =  cntrl_ptr->latency_us();
		}
#if BRPC_HANDLE_DEBUG
	std::cout << TRANSFER_DEBUG_HEADER << "ASYNC(not done) REQUIRE LOG SUCCESS " << std::endl;
#endif
	}
	else
	{
		if(latency_ptr != NULL)
		{
			*latency_ptr = cntrl_ptr->latency_us();
		}
		std::cout  << TRANSFER_ERROR_HEADER << "ASYNC REQUIRE LOG ERROR ! " << cntrl_ptr->ErrorText() << std::endl;
		return -1;		
	}
	return 1;
}

/**
 * Debug functions
 * **/
void debug_print_SendRequest(const MMLP_BRPC::LogSendRequest & res)
{
    std::cout << "MMLP_BPRC::LogSendRequest "<< std::endl << "{" << std::endl;
    std::cout << "  TrxID : " << res.trxid() << std::endl;
    std::cout << "  is_valid : " << res.is_valid() << std::endl;
    std::cout << "  TrxLogMsg : " << std::endl <<"      [ " << std::endl;
    for(int i = 0 ; i < res.log_msg_size() ; i++)
    {
       std::cout << "       ( type:" << res.log_msg(i).type() << "," ;
       std::cout << " space_id:" <<  res.log_msg(i).space_id() << ",";
       std::cout << " page_no:" << res.log_msg(i).page_no() << ",";
       std::cout << " offset:" << res.log_msg(i).offset() << "," ;
       std::cout << " rec_size:" << res.log_msg(i).rec().size() << " );" << std::endl;
    }
    std::cout << "      ]" << std::endl;
    std::cout <<"}" << std::endl;
}
