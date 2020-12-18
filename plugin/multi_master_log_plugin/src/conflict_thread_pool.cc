#include "conflict_thread_pool.h"
#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>
#include "conflict_thread_pool.h"
#include "mmlp_type.h"
#include "trx_log.h"

ConflictThreadEnv::ConflictThreadEnv()
{
	thd_ptr =  new std::thread(*this);
}

ConflictThreadEnv::~ConflictThreadEnv()
{
	delete thd_ptr;
}

void ConflictThreadEnv::operator()()
{
	ConflictTask cur_task;
	bool cur_res = false;
	while (1) 
	{
		if(task_queue.empty())
		{
			std::unique_lock<std::mutex> lck(task_mutex);
			task_thread_cond.wait(lck);
		}
		else
		{
			{
				std::unique_lock<std::mutex> lck(task_mutex);
				do
				{
					// pop conflict task 
					cur_task = task_queue.front();
					task_queue.pop();
					cur_res = task_state_map[cur_task.cur_log->get_trxID()]->result.load();
					task_state_map[cur_task.cur_log->get_trxID()]->count.fetch_sub(1);
				} while(cur_res);
			}
		  cur_res = (*verifyFunc)(cur_task.cur_log,cur_task.ref_log);
			if(cur_res == true)
			{
				task_state_map[cur_task.cur_log->get_trxID()]->result.store(cur_res);
			}
		}
	}
}

int ConflictThreadPool::init(ConflictVerifyFunction * verify_fun)
{
	ConflictThreadEnv::verifyFunc = verify_fun;

	for (int i = 0 ; i < DEFAULT_MAX_THREAD_POOL_CAPACITY ; i++ )
	{
		thread_vector.push_back(new ConflictThreadEnv);
	}
}

ConflictThreadPool::~ConflictThreadPool()
{
	for (auto it = thread_vector.begin(); it != thread_vector.end() ; it++)
	{
		delete *it;
	}
	thread_vector.clear();
}

int ConflictThreadPool::wait_for_result(TrxLog *cur_trx_log, std::vector<TrxLog *> &ref_list)
{
	{
		std::unique_lock<std::mutex> lck(ConflictThreadEnv::task_mutex);

		for (auto it = ref_list.begin(); it != ref_list.end(); it++)
		{
			add_task(cur_trx_log, *it);
		}
	}

	std::condition_variable trx_cond;
  std::mutex trx_mutex;

	while(ConflictThreadEnv::task_state_map[cur_trx_log->get_trxID()]->count >= 0)
	{	
		std::unique_lock<std::mutex> trx_lck(trx_mutex);
		trx_cond.wait(trx_lck);
	}

	{
		std::unique_lock<std::mutex> lck(ConflictThreadEnv::task_mutex);
		delete ConflictThreadEnv::task_state_map[cur_trx_log->get_trxID()];
		ConflictThreadEnv::task_state_map.erase(cur_trx_log->get_trxID());
	}
	return 1;
}

int ConflictThreadPool::add_task(TrxLog * cur_trx_log , TrxLog * ref_trx_log)
{
	TrxID cur_id = cur_trx_log->get_trxID();
 	ConflictTask tempTask;
	tempTask.cur_log = cur_trx_log;
	tempTask.ref_log = ref_trx_log;
	if(ConflictThreadEnv::task_state_map.find(cur_id) == ConflictThreadEnv::task_state_map.end())
	{
		ConflictThreadEnv::task_state_map.emplace(cur_id,new TaskState);
		ConflictThreadEnv::task_state_map[cur_id]->count.store(0);
		ConflictThreadEnv::task_state_map[cur_id]->result.store(false);
		ConflictThreadEnv::task_state_map[cur_id]->trx_cond_ptr = nullptr;
	}
	ConflictThreadEnv::task_queue.push(tempTask);
	ConflictThreadEnv::task_state_map[cur_id]->count.fetch_add(1);
	return 1;
}


