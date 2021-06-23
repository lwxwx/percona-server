
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
	for(auto part_it = end_id_waiter_map.begin();part_it != end_id_waiter_map.end(); part_it++)
	{
		for(auto map_it = part_it->second.begin(); map_it != part_it->second.end(); map_it++)
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

int TrxLogHoleSet::add_local_hole(UnifID trx_id) 
{
//std::cout << "Local Hole ID " << trx_id << std::endl;
  std::unique_lock<std::mutex> hole_set_lock(hole_mutex);
  local_hole_set[trx_id.p_id].insert(trx_id.own_id);
  hole_set[trx_id.p_id].insert(trx_id.own_id);
  return 1;
}

int TrxLogHoleSet::find_hole(UnifID trx_id)
{
	std::unique_lock<std::mutex> hole_set_lock(hole_mutex);
	if(hole_set[trx_id.p_id].find(trx_id.own_id) == hole_set[trx_id.p_id].end() && local_hole_set[trx_id.p_id].find(trx_id.own_id) == local_hole_set[trx_id.p_id].end())
	{
		//std::cout << "********************************Find no local id hole::" << trx_id <<std::endl;
		hole_set[trx_id.p_id].insert(trx_id.own_id);
		return 1;
	}
	else
	{
		return 0;
	}
}

int TrxLogHoleSet::wait_hole(UnifID end_id)
{
	if(DEBUG_CODE != 0)
	{
		std::cout << "wait hhh " << end_id << std::endl;
	}
	std::unique_lock<std::mutex>  hole_set_unique_lock(hole_mutex);

	if(end_id_waiter_map[end_id.p_id].find(end_id.own_id) == end_id_waiter_map[end_id.p_id].end())
	{
		end_id_waiter_map[end_id.p_id][end_id.own_id] = get_unused_waiter();
	}
	// wait trx_id <= end_id
	// end_id will wait itself
	int wait_count = 0;
	while(!hole_set[end_id.p_id].empty() &&  *hole_set[end_id.p_id].begin() <= end_id.own_id)
	{
		if(DEBUG_CODE != 0)
		{
			std::cout << "wait for [" << end_id.p_id << "-" << *hole_set[end_id.p_id].begin() <<"] hole_set part" << end_id.p_id  << " aready have hole : " ;
			for(auto itt : hole_set[end_id.p_id])
				std::cout << itt  <<" ";
			std::cout << std::endl;
		}
		if(wait_count > 3)
		{
			std::cout  << " [@TrxLogHole Wait ERROR] : wait count > 3 , block on the hole of end_id:" << end_id  << std::endl;
		//	return -1;
		}
		end_id_waiter_map[end_id.p_id][end_id.own_id]->cond.wait_for(hole_set_unique_lock,std::chrono::seconds(5));
		wait_count++;
	}

	return 1;
}

int TrxLogHoleSet::complement_hole(UnifID hole_id) 
{
// std::cout << "Complement Hole ID : " << hole_id << std::endl;
  std::unique_lock<std::mutex> hole_set_unique_lock(hole_mutex);

  if (hole_set[hole_id.p_id].find(hole_id.own_id) != hole_set[hole_id.p_id].end())
  {
    hole_set[hole_id.p_id].erase(hole_id.own_id);
  }
  else
  {
	  return 0;
  }

  if(local_hole_set[hole_id.p_id].find(hole_id.own_id) == local_hole_set[hole_id.p_id].end())
  {
	  local_hole_set[hole_id.p_id].erase(hole_id.own_id);
  }

  auto it = end_id_waiter_map[hole_id.p_id].begin();
  if(DEBUG_CODE != 0)
  {
	  std::cout << "when complete " <<hole_id << " , end_id_waiter_map " << hole_id.p_id << "have size = " << end_id_waiter_map[hole_id.p_id].size();
	  for(auto &tmpit : end_id_waiter_map[hole_id.p_id])
	  {
		  std::cout << "," << tmpit.first;
	  }
	  std::cout << " and hole set : ";
	  for(auto &tmpit : hole_set[hole_id.p_id])
	  	std::cout  << "," << tmpit;
	  std::cout << std::endl;
  }
  while(it != end_id_waiter_map[hole_id.p_id].end() && it->first < *hole_set[hole_id.p_id].begin())
  {
	  if(DEBUG_CODE != 0)
	  {
		  std::cout << "get hole [" << hole_id.p_id << "-" << it->first << "]" << std::endl;
	  }
	  it->second->cond.notify_all();
	  restore_unused_waiter(it->second);
	  end_id_waiter_map[hole_id.p_id].erase(it);
	  it++;
  }

  return 1;
}

int TrxLogHoleSet::notify_hole_get(UnifID tmp_id)
{
	end_id_waiter_map_lock_list[tmp_id.p_id].lock();
	end_id_waiter_map[tmp_id.p_id][tmp_id.own_id]->cond.notify_all();
	restore_unused_waiter(end_id_waiter_map[tmp_id.p_id][tmp_id.own_id]);
	end_id_waiter_map[tmp_id.p_id].erase(tmp_id.own_id);
	end_id_waiter_map_lock_list[tmp_id.p_id].unlock();
}
int TrxLogHoleSet::erase_hole(UnifID hole_id)
{
	try{
	std::unique_lock<std::mutex> hole_set_unique_lock(hole_mutex);

	if (hole_set[hole_id.p_id].find(hole_id.own_id) != hole_set[hole_id.p_id].end())
	{
		hole_set[hole_id.p_id].erase(hole_id.own_id);
	}
	else
	{
	  return 0;
	}
	}
	catch(std::exception & e)
	{
		std::cout << "notify error " << e.what() << std::endl;
	}
}