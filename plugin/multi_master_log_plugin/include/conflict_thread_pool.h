#ifndef  CONFLICT_THREAD_POOL_HEADER
#define CONFLICT_THREAD_POOL_HEADER

#include <atomic>
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
	TrxLog * cur_log;  // id of current transaction
	ConstTrxlogList task_content;
	std::condition_variable * task_finish_cond_ptr;
	bool * task_result;
};

class ConflictVerifyFunction
{
	public:
		virtual bool operator()(const TrxLog * ref_log,const TrxLog * cur_log) = 0;
};
// struct ConflictThreadEnv
// {
//   std::thread thd;
//   std::condition_variable idle_cond;
//   std::condition_variable task_finish_cond;
//   std::mutex idle_mutex;
//   uint32_t id;
//   std::queue<ConflictTask> task_queue;
// };

class ConflictThreadEnv
{
	public:
		ConflictThreadEnv();
		~ConflictThreadEnv();
		void operator()();

		static void task_execute(ConflictTask * task);
		static ConflictVerifyFunction * verifyFunc; 
	private:
		std::thread * thd_ptr;
		std::condition_variable idle_cond;
		std::mutex state_mutex;
		std::queue<ConflictTask> task_queue;
		std::atomic<bool> thread_destory_flag;
};

class ConflictThreadPool
{
	public:
		int init(uint32_t capacity,ConflictVerifyFunction * verify_fun); // start thread
		int distribute_task(TrxLog *cur_trx_log, ConstTrxlogList & task_content, uint32_t require_threads = 0); // 0 = max_capacity
		int destory_pool();
		~ConflictThreadPool();
	private:
		uint32_t max_capacity;
		
		//TODO : 多线程结果通知，参考workflow
		//std::map<TrxID,bool> result_map;
		//// task queue
		//// thread array
};
		//
#endif

	
