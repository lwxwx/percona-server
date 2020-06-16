/*
 * @Author: wei
 * @Date: 2020-06-16 10:05:03
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-06-16 10:15:17
 * @Description: plugin_interface.cc
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/src/plugin_interface.cc
 */

#include "plugin_interface.h"

int mml_plugin_trx_start()
{
    plugin_trx_info_ptr->trx_started();
}

int mml_plugin_mtr_redo_collect(redolog_lsn_t start_lsn, redolog_lsn_t end_lsn,void * log_buf)
{
    plugin_trx_info_ptr->mtr_redolog_collect(start_lsn, end_lsn,log_buf);
}

int mml_plugin_trx_commit(trx_t * trx_ptr)
{
    plugin_trx_info_ptr->trx_commiting(trx_ptr->no);
}