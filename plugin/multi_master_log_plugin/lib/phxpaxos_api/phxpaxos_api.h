/*
 * @Author: wei
 * @Date: 2020-06-25 20:11:52
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-07-14 09:22:30
 * @Description: file content
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/lib/phxpaxos_api/phxpaxos_api.h
 */
#ifndef PHXPAXOS_INTERFACE_HEADER
#define PHXPAXOS_INTERFACE_HEADER

#include "phxpaxos/node.h"
#include <string>

#include "phxpaxos/sm.h"
#include "phxpaxos/options.h"

#include <stdint.h>

class PhxAPISMCtx
{
public:
    int iExecuteRet;
    //std::string sEchoRespValue;

    PhxAPISMCtx()
    {
        iExecuteRet = -1;
    }
};

class PhxAPISM : public phxpaxos::StateMachine
{
    public:
    bool Execute(const int iGroupIdx, const uint64_t llInstanceID,
            const std::string & sPaxosValue, phxpaxos::SMCtx * poSMCtx);

    const int SMID() const { return 1; }

};

class PhxAPIServer
{
    public:
    PhxAPIServer();
    ~PhxAPIServer();

    int init(std::string & local_str,std::string & peers_str);

    int RunPaxos();
    int propose(std::string & message,uint64_t & no);

    private:
    int makeLogPath(std::string & path);

    private:
    phxpaxos::NodeInfo local_info;
    phxpaxos::NodeInfoList peers_info;

    phxpaxos::Node * m_poPaxosNode;
    PhxAPISM api_sm;
};

#endif