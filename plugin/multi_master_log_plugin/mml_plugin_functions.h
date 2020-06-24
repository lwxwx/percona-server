/*
 * @Author: wei
 * @Date: 2020-06-18 09:52:33
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-06-24 09:40:41
 * @Description: file content
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/mml_plugin_functions.h
 */

#ifndef MML_PLUGIN_FUNCTIONS_H
#define MML_PLUGIN_FUNCTIONS_H

#include "include/mmlp_type.h"

extern int mml_plugin_interface_active;

extern int (*mml_plugin_trx_start_ptr)();
extern int (*mml_plugin_mtr_redo_record_add_ptr)(plugin_mlog_id_t type,plugin_space_id_t space_id,plugin_page_no_t page_no,plugin_page_offset_t offset);
extern int (*mml_plugin_mtr_redo_record_new_ptr)(size_t size);
extern int (*mml_plugin_wr_trx_commit_ptr)(TrxID id);

#endif