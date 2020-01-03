#include "multi_master_handler.h"

//API pointer
MultiMasterAPI * multi_master_api = new MultiMasterAPI;

/***
 *  MultiMasterAPI  implementation
 * **/
MultiMasterAPI::MultiMasterAPI()
{
    is_plugin_load = false;
}

bool MultiMasterAPI::is_loaded()
{
    return is_plugin_load;
}

void MultiMasterAPI::register_API()
{
    is_plugin_load = true;
}
