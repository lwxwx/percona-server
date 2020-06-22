/*
 * @Author: wei
 * @Date: 2020-06-18 10:10:01
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-06-20 09:44:34
 * @Description: file content
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/mml_plugin_functions.cc
 */

#include "mml_plugin_functions.h"

int mml_plugin_interface_active = -1;

int (*mml_plugin_trx_start_ptr)() = NULL;
int (*mml_plugin_mtr_redo_record_add_ptr)(plugin_mlog_id_t type,plugin_space_id_t space_id,plugin_page_no_t page_no,plugin_page_offset_t offset) = NULL;
int (*mml_plugin_mtr_redo_record_new_ptr)(size_t size);
int (*mml_plugin_wr_trx_commit_ptr)(TrxID id) = NULL;