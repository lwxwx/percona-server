#include "conflict_thread_pool.h"
#include <mutex>
#include "trx_log.h"

ConflictThreadEnv::ConflictThreadEnv()
{

}

ConflictThreadEnv:~ConflictThreadEnv()
{

}

ConflictThreadEnv::operator()()
{
	while (1) 
	{
		{
			std::unique_lock<std::mutex> lck(state_mutex);
			idle_cond.wait(lck,[]{return thread_finish_flag.load() || !task_queue.empty(); });
		}
		if(thread_finish_flag.load() && task_queue.empty())
		{
			break;
		}

		ConflictTask * task_ptr = &task_queue.front();
		task_execute(task_ptr);
		if(task_ptr->task_finish_cond_ptr != NULL)
		{
			task_ptr->task_finish_cond_ptr->notify_all();
		}
		task_queue.pop();

	}
}

void ConflictThreadEnv::task_execute(ConflictTask *task)
{
	auto it = task->task_content.begin();
	bool verify_result = false;
	for( ; it != task->task_content.end(); it++)
	{
		verify_result = (*verifyFunc)(task->cur_log,*it);
		if(verify_result)
		{
			break;
		}
	}
	(*task->task_result) = verify_result;
}

int ConflictThreadPool::init(uint32_t capacity ,ConflictVerifyFunction * verify_fun)
{
	verifyFunc = verify_fun;
	max_capacity = capacity;
	thread_vector.resize(capacity);
	for (int i = 0; i < capacity; i++)
	{
		thread_vector[i].thd_id = i;
	}
	return 1;
}

ConflictThreadPool::~ConflictThreadPool()
{
	thread_vector.clear();
}

int ConflictThreadPool::distribute_task(TrxLog * cur_trx_log,ConstTrxlogList &task_content,uint32_t require_threads)
{
	if(require_threads <= 0)
	{
		require_threads = max_capacity;
	}
	uint32_t task_len = task_content.size();
	if(task_len <= 2*require_threads)
	{
		//TODO verify in local thread
	}
	else
	{
		uint32_t unit_len = task_len / require_threads;	

	}

	return 1;
}
