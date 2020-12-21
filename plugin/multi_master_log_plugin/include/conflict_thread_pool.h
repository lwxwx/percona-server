#ifndef  CONFLICT_THREAD_POOL_HEADER 
#define CONFLICT_THREAD_POOL_HEADER 
// #include <bits/stdint-uintn.h>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <list>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include "mmlp_type.h"
#include "trx_log.h"

#define DEFAULT_MAX_THREAD_POOL_CAPACITY 8
#define DEFAULT_TASK_UNIT_LEN 5

typedef std::list<const TrxLog *> ConstTrxlogList;

struct ConflictTask
{
	TrxLog * cur_log;  // id of current transaction
	TrxLog * ref_log;
};

struct TaskState
{
	std::atomic_uint32_t count; // only when count <= 0, TaskState can be delete;
	std::atomic_bool result;
	std::condition_variable * trx_cond_ptr;
};

class ConflictVerifyFunction
{
	public:
		virtual bool operator()(const TrxLog * cur_log,const TrxLog * ref_log) = 0;
};

class ConflictThreadEnv
{
	public:
		ConflictThreadEnv();
		~ConflictThreadEnv();
		void operator()();

		//after verify , modify the result, check task counts of GTSN , delete from map and notify trx thread when finish
		//Not init in construct function
		static ConflictVerifyFunction * verifyFunc; 
		static std::mutex task_mutex;		
		static std::condition_variable task_thread_cond;
		static std::queue<ConflictTask> task_queue; 
		static std::map<TrxID, TaskState *> task_state_map;
	private:
		std::thread * thd_ptr;
};

class ConflictThreadPool
{
	public:
		int init(ConflictVerifyFunction * verify_fun); // start thread
		int wait_for_result(TrxLog * cur_trx_log, std::vector<TrxLog *> & ref_list);
		~ConflictThreadPool();
	
	private:
		std::vector<ConflictThreadEnv *> thread_vector;
		
		int add_task(TrxLog * cur_trx_log, TrxLog * ref_trx_log);
};
#endif
	
	
