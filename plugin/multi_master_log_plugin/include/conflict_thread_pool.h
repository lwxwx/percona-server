#ifndef  CONFLICT_THREAD_POOL_HEADER
#define CONFLICT_THREAD_POOL_HEADER

#include <condition_variable>
#include <cstdint>
#include <list>
#include <mutex>
#include <queue>
#include <thread>
#include "mmlp_type.h"
#include "trx_log.h"
#define DEFAULT_MAX_THREAD_POOL_CAPACITY 8

typedef std::list<const TrxLog *> ConstTrxlogList;

struct ConflictTask
{
	TrxID cur_trx_id;  // id of current transaction
	ConstTrxlogList task_content;
};

struct ConflictThreadEnv
{
	std::thread thd;
	std::condition_variable idle_cond;
	std::condition_variable task_finish_cond;
	std::mutex idle_mutex;
	uint32_t id;
	std::queue<ConflictTask> task_queue;
};

class ConflictVerifyFunction
{
	public:
		virtual void operator()(const TrxLog * ref_log,const TrxLog * cur_log) = 0;
};

class ConflictThreadPool
{
	public:
		int init(uint32_t capacity); // start thread
		int distribute_task(ConstTrxlogList & task_content,uint32_t require_threads = 0); // 0 = max_capacity
		int destory_pool();
	private:
		uint32_t max_capacity;
		
		//TODO : 多线程结果通知，参考workflow
		//std::map<TrxID,bool> result_map;
		//// task queue
		//// thread array
};
		//
#endif

	
