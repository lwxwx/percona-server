
#include "trx_log_hole.h"
#include <chrono>
#include <mutex>
#include <iostream>
#include <ostream>
#include "mmlp_type.h"

TrxLogHoleSet::~TrxLogHoleSet()
{
	for(auto it = unused_waiter_list.begin(); it != unused_waiter_list.end(); it++)
	{
		delete *it;
	}
	for(auto map_it = end_id_waiter_map.begin();map_it != end_id_waiter_map.end(); map_it++)
	{
		delete map_it->second;
	}
}

HoleWaiter * TrxLogHoleSet::get_unused_waiter()
{
	if(unused_waiter_list.empty())
	{
		return new HoleWaiter;
	}
	std::lock_guard<std::mutex> pool_lock(waiter_list_mutex);
	HoleWaiter *  tmp =  unused_waiter_list.front();
	unused_waiter_list.pop_front();
	return tmp;
}

void TrxLogHoleSet::restore_unused_waiter(HoleWaiter * target)
{
	std::lock_guard<std::mutex> pool_lock(waiter_list_mutex);
	//target->trx_id = 0;
	unused_waiter_list.push_back(target);
}

int TrxLogHoleSet::add_local_hole(TrxID trx_id) 
{
//std::cout << "Local Hole ID " << trx_id << std::endl;
  std::unique_lock<std::mutex> hole_set_lock(hole_mutex);
  local_hole_set.insert(trx_id);
  hole_set.insert(trx_id);
  return 1;
}

int TrxLogHoleSet::find_hole(TrxID trx_id)
{
	std::unique_lock<std::mutex> hole_set_lock(hole_mutex);
	if(hole_set.find(trx_id) == hole_set.end() && local_hole_set.find(trx_id) == local_hole_set.end())
	{
		//std::cout << "********************************Find no local id hole::" << trx_id <<std::endl;
		hole_set.insert(trx_id);
		return 1;
	}
	else
	{
		return 0;
	}
}

int TrxLogHoleSet::wait_hole(TrxID end_id)
{
	std::unique_lock<std::mutex>  hole_set_unique_lock(hole_mutex);

	if(end_id_waiter_map.find(end_id)==end_id_waiter_map.end())
	{
		end_id_waiter_map[end_id] = get_unused_waiter();
	}
	// wait trx_id <= end_id
	// end_id will wait itself
	int wait_count = 0;
	while(!hole_set.empty() &&  *hole_set.begin() <= end_id)
	{
		if(wait_count > 3)
		{
			std::cout  << " [@TrxLogHole Wait ERROR] : wait count > 3 , block on the hole of end_id:" << end_id  << std::endl;
		//	return -1;
		}
		end_id_waiter_map[end_id]->cond.wait_for(hole_set_unique_lock,std::chrono::seconds(5));
		wait_count++;
	}

	return 1;
}

int TrxLogHoleSet::complement_hole(TrxID hole_id) 
{
// std::cout << "Complement Hole ID : " << hole_id << std::endl;
  std::unique_lock<std::mutex> hole_set_unique_lock(hole_mutex);

  if (hole_set.find(hole_id) != hole_set.end())
  {
    hole_set.erase(hole_id);
  }
  else
  {
	  return 0;
  }

  if(local_hole_set.find(hole_id) == local_hole_set.end())
  {
	  local_hole_set.erase(hole_id);
  }

  auto it = end_id_waiter_map.begin();
  while(it != end_id_waiter_map.end() && it->first < *hole_set.begin())
  {
	  it->second->cond.notify_all();
	  restore_unused_waiter(it->second);
	  end_id_waiter_map.erase(it);
	  it++;
  }

  return 1;
}
