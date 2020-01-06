#ifndef MULTI_MASTER_HANDLER_HEADER
#define MULTI_MASTER_HANDLER_HEADER


/****************************************************************************************************
 * developer Macro
 ****************************************************************************************************/
//macro for wei
#define MULTI_MASTER_WEI_NORMAL


/****************************************************************************************************
 * Multi Master API Type
 ****************************************************************************************************/
//typedef XXX  XXXX
extern EventMessageHandle * event_msg_handle_ptr;


/****************************************************************************************************
 * Multi Master API
 ****************************************************************************************************/
class MultiMasterAPI
{
    public:

    MultiMasterAPI();

    /*
    IF(function ptr == 0 ) : not load this api function
    API function return value
        + >0 success
        + <0 error
    */

    private:
};

extern MultiMasterAPI * multi_master_api;

// ------------------- function invoke marco
// inherit from percona-server/sql/rpl_handler.h  #define RUN_HOOK(group, hook, args)
#define INVOKER_MM_API(function,args) \
    (multi_master_api->function ?  *(multi_master_api->function) args : 0)

#define REGISTER_MM_API(function_api,function) \
    (multi_master_api->function_api = function)


#endif