/*
 * @Author: wei
 * @Date: 2020-07-07 15:08:51
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-07-15 21:32:35
 * @Description: file content
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/include/id_allocate.h
 */

#ifndef GLOBAL_TRX_ID_ALLOCATE_HEADER
#define GLOBAL_TRX_ID_ALLOCATE_HEADER

#include <condition_variable>
#include <brpc/channel.h>
#include "id.pb.h"
#define PHXPAXOS_ID_COMPLIE 0

#include "mmlp_type.h"

#if PHXPAXOS_ID_COMPLIE
#include "phxpaxos_api.h"
#endif
#include <queue>
#include <atomic>
#include <thread>
#include <mutex>
#include <string>
#include <sstream>

#define  REMOTE_NODE_GEN_DEBUG "[RemoteNodeTrxIdGen DEBUG] : "
#define  REMOTE_NODE_GEN_ERROR "[RemoteNodeTrxIdGen ERROR] : "

enum ID_FACTORY_TYPE
{
    PHXPAXOS_PROPOSE_SYNC = 16,
    FROM_REMOTE_NODE_ASYNC = 1,
    SLICE_ID_GEN_SYNC = 0
};

// typedef int REQUEST_HANDLE();
// typedef int CACHE_ID();

class GlobalTrxIdGen
{
    public:
    virtual int init() = 0;
    virtual int handle_request() = 0;
    virtual TrxID get_id() = 0;
};


class GlobalTrxIDFactory
{
    private:
    ID_FACTORY_TYPE factory_type;

    //请求队列
    //std::atomic_uint64_t request_count;

    //std::mutex cache_id_mutex;

    GlobalTrxIdGen * global_id_gen;

    int setFactoryType(ID_FACTORY_TYPE type);

    public:
    int init(ID_FACTORY_TYPE type);
    ID_FACTORY_TYPE getFactoryType();

    int requestGlobalTrxID();
    TrxID getGlobalTrxID();
};

#if PHXPAXOS_ID_COMPLIE
class PhxPaxosTrxIdGen_SYNC : public GlobalTrxIdGen
{
    private:
    //std::queue<TrxID> cache_id;
    PhxAPIServer m_paxos;

    public:
    int init();
    int handle_request(){return 1;};
    TrxID get_id();
};
#endif

class RemoteNodeTrxIdGen : public GlobalTrxIdGen
{
    private:
    std::set<TrxID> cache_queue;
	std::mutex cache_queue_mutex;
	std::condition_variable cache_id_condition;
	
	std::string server_addr;
	brpc::Channel channel;
	IDIncrement::IDService_Stub * stub_ptr;
    public:
    int init();
    int handle_request();
    TrxID get_id();

	int cacheIncrementID(TrxID id);
};

void idIncrementResponseHanle(brpc::Controller * cntrl,IDIncrement::IDResponse * response_ptr,RemoteNodeTrxIdGen * id_gen);

class SliceTrxIdGen_SYNC : public GlobalTrxIdGen
{
    private:
    //std::queue<TrxID> cache_id;
    uint32_t node_no;
    uint32_t slice_len;
    uint64_t slice_count;

    public:
    int init();
    int handle_request(){return 1;};
    TrxID get_id();
};

#endif
