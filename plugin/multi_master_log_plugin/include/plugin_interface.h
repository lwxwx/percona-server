/*
 * @Author: wei
 * @Date: 2020-06-16 09:37:36
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-06-20 09:41:56
 * @Description: functions called in mysql
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/include/plugin_interface.h
 */

#ifndef PLUGIN_INTERFACE_HEADER
#define PLUGIN_INTERFACE_HEADER

#include "trx_info.h"

/*plugin mysql interface
    - Redo Log Collect
        - trx start flag
        - mtr redo log
        - trx commit flag
    - Read_Write Set Collect
*/

/*Redo Log Collect*/
int mml_plugin_trx_start();

int mml_plugin_mtr_redo_record_add(plugin_mlog_id_t type,plugin_space_id_t space_id,plugin_page_no_t page_no,plugin_page_offset_t offset);

int mml_plugin_mtr_redo_record_new(size_t size);

int mml_plugin_wr_trx_commit(TrxID id);


#endif /* PLUGIN_INTERFACE_HEADER*/