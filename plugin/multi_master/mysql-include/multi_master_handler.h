#ifndef MULTI_MASTER_HANDLER_HEADER
#define MULTI_MASTER_HANDLER_HEADER


/****************************************************************************************************
 * developer Macro
 ****************************************************************************************************/
//macro for wei
#define MULTI_MASTER_WEI_NORMAL


/****************************************************************************************************
 * Multi Master API
 ****************************************************************************************************/
class MultiMasterAPI
{
    public:
    MultiMasterAPI();

    bool is_loaded();
    void register_API();

    /* API function return value
        + >0 success
        + <0 error
    */

    private:
    bool is_plugin_load;
};

extern MultiMasterAPI * multi_master_api;

// ------------------- function invoke marco
// inherit from percona-server/sql/rpl_handler.h  #define RUN_HOOK(group, hook, args)
#define INVOKER_MM_API(function,args) \
    (multi_master_api->is_loaded() ?  multi_master_api->function args : 0)

#endif