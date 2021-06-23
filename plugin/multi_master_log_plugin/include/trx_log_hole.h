/*
 * @Author: wei
 * @Date: 2020-07-28 18:12:49
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-07-28 20:08:04
 * @Description: file content
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/include/trx_log_hole.h
 */

#ifndef MMLP_TRX_LOG_HOLE_HEADER
#define MMLP_TRX_LOG_HOLE_HEADER

#include "mmlp_type.h"
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <set>
#include <map>
#include <list>
#include <unordered_map>


struct HoleWaiter
{
    //TrxID trx_id;
    std::condition_variable cond;
};


class TrxLogHoleSet
{
    private:
	// std::set<TrxID> local_hole_set;
    // std::set<TrxID> hole_set;
    // std::map<TrxID,HoleWaiter *> end_id_waiter_map;
    std::map<int32_t,std::set<int64_t>> local_hole_set;
    std::map<int32_t,std::set<int64_t>> hole_set;
    std::map<std::int32_t,std::map<int64_t,HoleWaiter *>> end_id_waiter_map;
    std::mutex hole_mutex;
    std::map<std::int32_t,std::mutex> end_id_waiter_map_lock_list;
	//std::mutex local_hole_mutex;

	std::mutex waiter_list_mutex;
    std::list<HoleWaiter *> unused_waiter_list;

	HoleWaiter * get_unused_waiter();
	void restore_unused_waiter(HoleWaiter * target);

    public:
	~TrxLogHoleSet();
	// add one local hole id
	// int add_local_hole(TrxID);
    int add_local_hole(UnifID);
	//find one hole , return 0 : don't send log require message
    // int find_hole(TrxID);
    int find_hole(UnifID);
	//wait hole before end_id
    // int wait_hole(TrxID end_id);
    int wait_hole(UnifID end_id);
	//complement_hole a hole when recived a log require response or commitd a local trx 
    // int complement_hole(TrxID);
    int complement_hole(UnifID);
    int notify_hole_get(UnifID);
    int erase_hole(UnifID);
};

#endif
