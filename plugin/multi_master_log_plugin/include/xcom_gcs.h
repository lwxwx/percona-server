/*
 * @Author: wei
 * @Date: 2020-06-23 09:25:20
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-06-23 13:34:56
 * @Description: file content
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/include/xcom_gcs.h
 */

#ifndef MMLP_XCOM_GCS_HEADER
#define MMLP_XCOM_GCS_HEADER

#include "gcs_interface.h"
#include "debug.h"
#include "easylogger.h"

class XcomGcs
{
private:
    Gcs_interface * gcs_interface;
    Gcs_group_identifier * group_id;
    Gcs_control_interface * ctrl_if;
    Gcs_communication_interface * comm_if;
    Gcs_communication_event_listener * listener_ptr;
    int listener_handle;

public:
    XcomGcs(/* args */);
    ~XcomGcs();

    int init();

    /*Test code*/
    int send_test_message(const char * data,int len);
    class DEBUG_Gcs_communication_event_listener: Gcs_communication_event_listener
    {
        void on_message_received(const Gcs_message &message) const
        {
            //log recive
            std::string origin_id = message.get_origin().get_member_id();
            std::string destin_id = message.get_destination()->get_group_id();

            const char * data = (const char *)message.get_message_data().get_payload();
            uint64_t len = message.get_message_data().get_payload_length();

            #if DEBUG_XCOM_TEST
                std::stringstream ss;
                ss << LOG_DIR << XCOM_LOG_DIR << XCOM_LOG_FILE_NAME << LOG_FILE_SUFFIX;
                std::stringstream mess;
                mess<<  "@Message from " << origin_id << " to " << destin_id <<" : " << std::string(data,len) << std::endl;
                EasyStringLog(ss.str(), origin_id,
                    mess.str(),EasyLogger::LOG_LEVEL::debug);
            #endif


        }
    }
};

#endif