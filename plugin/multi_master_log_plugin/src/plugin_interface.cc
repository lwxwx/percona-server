/*
 * @Author: wei
 * @Date: 2020-06-16 10:05:03
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-06-20 09:43:18
 * @Description: plugin_interface.cc
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/src/plugin_interface.cc
 */

#include "plugin_interface.h"

// int mml_plugin_interface_active = -1;

int mml_plugin_trx_start()
{
    return plugin_trx_info_ptr->trx_started();
}

int mml_plugin_mtr_redo_record_add(plugin_mlog_id_t type,plugin_space_id_t space_id,plugin_page_no_t page_no,plugin_page_offset_t offset)
{
    return plugin_trx_info_ptr->mtr_redolog_record_add(type,space_id,page_no,offset);
}

int mml_plugin_mtr_redo_record_new(size_t size)
{
    return plugin_trx_info_ptr->mtr_redolog_record_new(size);
}

int mml_plugin_wr_trx_commit(TrxID id)
{
    return plugin_trx_info_ptr->wr_trx_commiting(id);
}