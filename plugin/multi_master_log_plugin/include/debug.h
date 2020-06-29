/*
 * @Author: wei
 * @Date: 2020-06-16 15:25:53
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-06-26 22:03:21
 * @Description: Marco for debug
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/include/debug.h
 */

#ifndef DEBUG_HEADER
#define DEBUG_HEADER

#define LOG_DIR "/tmp/mmlp/"
#define LOG_FILE_SUFFIX ".log"
#define REDO_LOG_FILE_NAME "redo_collect_log"

// #define XCOM_LOG_DIR "xcom/"
// #define XCOM_LOG_FILE_NAME "xcom_debug"

#define DEBUG_REDO_LOG_COLLECT 0

#define DEBUG_PHXPAXOS_PRINT 0

#endif /* DEBUG_HEADER */