/*
 * @Author: wei
 * @Date: 2020-06-23 09:28:01
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-06-23 13:34:36
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

int  XcomGcs::init()
{
    if(!gcs_interface->is_initialized())
    {
        Gcs_interface_parameters param;
        // param.add_parameter("ip",ip);
        // param.add_parameter("port",std::to_string(port));
        //param.add_parameter("node_id",node_id);
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
    Gcs_member_identifier member_id = ctrl_if->get_local_member_identifier();

    Gcs_message * msg = new Gcs_message(member_id,*group_id,new Gcs_message_data(10));

    msg->get_message_data().append_to_payload((uchar *)data,len);

    comm_if->send_message(*msg);

    return 1;

}