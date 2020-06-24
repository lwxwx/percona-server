/*
 * @Author: wei
 * @Date: 2020-06-23 09:28:01
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-06-24 19:16:30
 * @Description: file content
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/src/xcom_gcs.cc
 */

#include "xcom_gcs.h"

XcomGcs::XcomGcs(/* args */)
{
    gcs_interface = Gcs_interface_factory::get_interface_implementation(enum_available_interfaces::XCOM);
}

XcomGcs::~XcomGcs()
{
    if(gcs_interface->is_initialized())
    {
        comm_if->remove_event_listener(listener_handle);
        ctrl_if->leave();
        gcs_interface->finalize();
        Gcs_interface_factory::cleanup(enum_available_interfaces::XCOM);
    }
}

int  XcomGcs::init(const char * g_name,const char * local,const char * peers)
{
    if(!gcs_interface->is_initialized())
    {
        Gcs_interface_parameters param;
        param.add_parameter("group_name",g_name);
        param.add_parameter("bootstrap_group", "false");
        param.add_parameter("local_node",local);
        param.add_parameter("peer_nodes",peers);
        //param.add_parameter("fragmentation", std::string("off"));
        gcs_interface->initialize(param);

        group_id = new Gcs_group_identifier("mmlp_group");

        ctrl_if = gcs_interface->get_control_session(*group_id);
        ctrl_if->join();

        comm_if = gcs_interface->get_communication_session(*group_id);

        listener_ptr = (Gcs_communication_event_listener*)new DEBUG_Gcs_communication_event_listener;

        listener_handle = comm_if->add_event_listener(*listener_ptr);

        return 1;
    }
    else
    {
        return 0;
    }
}


int XcomGcs::send_test_message(const char * data,int len)
{
    if(!ctrl_if->belongs_to_group())
    {
        enum_gcs_error error = ctrl_if->join();
        if(error != enum_gcs_error::GCS_OK)
        {
            #if XCOM_ERROR_LOG
                std::cout << "Join group before send test message failed" << std::endl;
            #endif
            return -1;
        }
    }

    Gcs_member_identifier member_id = ctrl_if->get_local_member_identifier();

    Gcs_message * msg = new Gcs_message(member_id,*group_id,new Gcs_message_data(10));

    msg->get_message_data().append_to_payload((uchar *)data,len);

    comm_if->send_message(*msg);

    return 1;

}