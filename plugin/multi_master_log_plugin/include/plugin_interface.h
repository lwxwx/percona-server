/*
 * @Author: wei
 * @Date: 2020-06-16 09:37:36
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-06-16 10:13:16
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

int mml_plugin_mtr_redo_collect(redolog_lsn_t start_lsn, redolog_lsn_t end_lsn,void * log_buf);

int mml_plugin_trx_commit(trx_t * trx_ptr);


#endif /* PLUGIN_INTERFACE_HEADER*/