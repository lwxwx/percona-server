/*
 * @Author: wei
 * @Date: 2020-07-14 07:24:06
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-07-23 09:38:58
 * @Description: file content
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/src/id_allocate.cc
 */

#include "id_allocate.h"
#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <mutex>
#include "brpc/channel.h"
#include "brpc/controller.h"
#include "brpc/options.pb.h"
#include "id.pb.h"
#include "mmlp_type.h"
#include <string>
#include <memory>
/**
 * class GlobalTrxIDFactory
 * **/
int GlobalTrxIDFactory::setFactoryType(ID_FACTORY_TYPE type)
{
    factory_type = type;
    return 1;
}

ID_FACTORY_TYPE GlobalTrxIDFactory::getFactoryType()
{
    return factory_type;
}

int GlobalTrxIDFactory::init(ID_FACTORY_TYPE type)
{
    setFactoryType(type);

    //request_count = 0;
    global_id_gen = NULL;
    if(factory_type == ID_FACTORY_TYPE::SLICE_ID_GEN_SYNC)
    {
        global_id_gen = new SliceTrxIdGen_SYNC;
    }
#if PHXPAXOS_ID_COMPLIE
    if(factory_type == ID_FACTORY_TYPE::PHXPAXOS_PROPOSE_SYNC)
    {
        global_id_gen = new PhxPaxosTrxIdGen_SYNC;
    }
#endif
	if(factory_type == ID_FACTORY_TYPE::FROM_REMOTE_NODE_ASYNC)
	{
		global_id_gen = new RemoteNodeTrxIdGen;
	}

    if(global_id_gen != NULL)
        global_id_gen->init();
    else
        return -1;

    return 1;
}

int GlobalTrxIDFactory::requestGlobalTrxID()
{
    global_id_gen->handle_request();
    return 1;
}

TrxID GlobalTrxIDFactory::getGlobalTrxID()
{
    return global_id_gen->get_id();
}

/**
 * class RemoteNodeTrxIdGen
 * **/
int RemoteNodeTrxIdGen::init()
{
	if(remote_id_server_addr != nullptr)
		server_addr = remote_id_server_addr;
	brpc::ChannelOptions options; 
	options.protocol = brpc::PROTOCOL_BAIDU_STD;
    options.connection_type = brpc::CONNECTION_TYPE_SINGLE;
	//options.max_retry = 5;
    if (channel.Init(server_addr.c_str(), &options) != 0) {
		std::cout << REMOTE_NODE_GEN_ERROR << "Fail to initialize channel"<< std::endl;;
        return -1;
    }
}

int RemoteNodeTrxIdGen::handle_request()
{
	uint64_t before_handle_request;
    if(DEBUG_REMOTE_ID_TIME != 0)
    {
        before_handle_request = (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())).count();
    }
 
	stub_ptr =  new IDIncrement::IDService_Stub(&channel);

	IDIncrement::IDRequest request;
	brpc::Controller * cntrl = new brpc::Controller;
	IDIncrement::IDResponse * response_ptr = new IDIncrement::IDResponse;

	request.set_message("request id");

	google::protobuf::Closure* done = brpc::NewCallback(&idIncrementResponseHanle,cntrl,response_ptr,this);
	stub_ptr->IDInc(cntrl, &request, response_ptr, done);

    if(DEBUG_REMOTE_ID_TIME != 0)
    {
        remote_id_handle_count++;
        remote_id_handle_time += (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())).count()- before_handle_request;
    }
}

void idIncrementResponseHanle(brpc::Controller * cntrl,IDIncrement::IDResponse * response_ptr,RemoteNodeTrxIdGen * id_gen)
{
    // std::unique_ptr makes sure cntl/response will be deleted before returning.
	std::unique_ptr<brpc::Controller> cntl_guard(cntrl);
	std::unique_ptr<IDIncrement::IDResponse> response_guard(response_ptr);
   
    if (cntrl->Failed()) {
		id_gen->handle_request();
		std::cout << REMOTE_NODE_GEN_ERROR  <<"Fail to send IDIncrementRequest, " << cntrl->ErrorText()<<std::endl;
        return;
    }
//   std::cout << REMOTE_NODE_GEN_DEBUG <<"Received id : "<<response_ptr->message()<<std::endl;
    // brpc::AskToQuit();
	id_gen->cacheIncrementID((TrxID)std::stoull(response_ptr->message()));
}

int RemoteNodeTrxIdGen::cacheIncrementID(TrxID id)
{
	std::unique_lock<std::mutex> cache_lock(cache_queue_mutex);
	cache_queue.insert(id);
	cache_id_condition.notify_one();
	return 1;
}

TrxID RemoteNodeTrxIdGen::get_id()
{

	uint64_t before_get_id;
    if(DEBUG_REMOTE_ID_TIME != 0)
    {
        before_get_id = (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())).count();
    }
//	int wait_count = 0;
	std::unique_lock<std::mutex> cache_lock(cache_queue_mutex);
	if(cache_queue.empty())
	{
//		handle_request();
		while(cache_queue.empty())
		{
			cache_id_condition.wait_for(cache_lock,std::chrono::seconds(5));	
			if(cache_queue.empty())// && wait_count >= 1)
			{
				std::cout << "IDIncrement get_id() Failed times, cache_queue.empty() is " << cache_queue.empty()  << std::endl;
				if(DEBUG_REMOTE_ID_TIME !=0 )
				{
					remote_id_over_wait_count++;
				}
			    handle_request();
				//wait_count  = 0;
				//assert(false);
			}
		//	wait_count++;
		}
	}
	
	TrxID id = *cache_queue.begin();
//	std::cout << "Get Remote ID of "  << id << std::endl;
	cache_queue.erase(cache_queue.begin());
    if(DEBUG_REMOTE_ID_TIME != 0)
	{
		remote_id_get_count++;
        remote_id_get_time += (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())).count()- before_get_id;
	}
	return id;
}


/**
 * class SliceTrxIdGen_SYNC
 * **/
int SliceTrxIdGen_SYNC::init()
{ slice_len = 0; std::string peers_str = std::string(brpc_peer_nodes_ptr); std::stringstream peers_ss(peers_str); std::string tmp; //std::cout <<"Peer Nodes : " << peers_str << std::endl; while(!peers_ss.eof())
    {
        getline(peers_ss,tmp,',');
     //   std::cout << "Line Node : " << tmp << std::endl;
        slice_len++;
    }

    node_no = slice_node_no;
    slice_count = 0;

    return 1;
}

TrxID SliceTrxIdGen_SYNC::get_id()
{
    uint64_t before_slice_cal = 0;
    if(DEBUG_SLICE_TIME != 0)
    {
        before_slice_cal = (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())).count();
    }
	
    slice_count++;
    TrxID id = node_no + slice_count * slice_len;

    if(DEBUG_SLICE_TIME != 0)
    {
        slice_gen_count++;
        slice_gen_time += (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())).count()- before_slice_cal;
    }

    return id;
}

#if PHXPAXOS_ID_COMPLIE
/**
 * class PhxPaxosTrxIdGen_SYNC
 * **/
int PhxPaxosTrxIdGen_SYNC::init()
{
    std::string local_str = std::string(phxpaxos_local_node_ptr);
    std::string peer_str = std::string(phxpaxos_peer_nodes_ptr);
    //std::cout << "phxpaxos init 1" << std::endl;
    m_paxos.init(local_str,peer_str);
    //std::cout << "phxpaxos init 2" << std::endl;
    m_paxos.RunPaxos();
    //std::cout << "phxpaxos init 3" << std::endl;

    return 1;
}

TrxID PhxPaxosTrxIdGen_SYNC::get_id()
{
    TrxID id = 0;
    bool has_conflict = false;
    std::string id_str = "";

    uint64_t before_propose = 0;
    if(DEBUG_PHXPAXOS_TIME != 0)
    {
        before_propose = (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())).count();
    }

    int ret = m_paxos.propose(id_str,id);

    // propose conflict
    while (ret == phxpaxos::PaxosTryCommitRet_Conflict)
    {
        has_conflict = true;
        ret = m_paxos.propose(id_str,id);
        //std::cout << "PhxPaxos Conflict" << std::endl;
    }

    if(has_conflict)
    {
        if(DEBUG_PHXPAXOS_TIME != 0)
        {
            phxpaxos_conflict_count++;
            phxpaxos_conflict_time +=  (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())).count() - before_propose;
        }
    }

    if(ret == phxpaxos::PaxosTryCommitRet_OK)
    {
        if(DEBUG_PHXPAXOS_TIME != 0)
        {
            phxpaxos_propose_count++;
            phxpaxos_propose_time +=  (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())).count() - before_propose;
        }
    }
    else
    {
        if(DEBUG_PHXPAXOS_TIME != 0)
            phxpaxos_other_count++;
    }

    return id;
}
#endif
