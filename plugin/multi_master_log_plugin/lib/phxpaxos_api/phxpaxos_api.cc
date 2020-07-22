/*
 * @Author: wei
 * @Date: 2020-06-25 20:31:35
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-07-14 09:24:39
 * @Description: file content
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/lib/phxpaxos_api/phxpaxos_api.cc
 */

#include "phxpaxos_api.h"
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "debug.h"

#include "mmlp_type.h"

using namespace phxpaxos;

bool PhxAPISM::Execute(const int iGroupIdx, const uint64_t llInstanceID,
            const std::string & sPaxosValue, phxpaxos::SMCtx * poSMCtx)
{
    if(DEBUG_PHXPAXOS_PRINT!=0)
    {
        printf("[SM Execute] ok, smid %d instanceid %lu value %s\n",
                 SMID(), llInstanceID, sPaxosValue.c_str());
    }
    //only commiter node have SMCtx.
    if (poSMCtx != nullptr && poSMCtx->m_pCtx != nullptr)
    {
        PhxAPISMCtx * ctx = (PhxAPISMCtx *)poSMCtx->m_pCtx;
        ctx->iExecuteRet = 0;
        // PhxEchoSMCtx * poPhxEchoSMCtx = (PhxEchoSMCtx *)poSMCtx->m_pCtx;
        // poPhxEchoSMCtx->iExecuteRet = 0;
        // poPhxEchoSMCtx->sEchoRespValue = sPaxosValue;
    }

    return true;
}

PhxAPIServer::PhxAPIServer()
{
    m_poPaxosNode = NULL;
}

int PhxAPIServer::init(std::string & local_str,std::string & peers_str)
{
     //local_info
    std::stringstream local_ss(local_str);
    std::string ip = "";
    std::string port = "";
    getline(local_ss,ip,':');
    getline(local_ss,port);
    int port_int = stoi(port);
    local_info.SetIPPort(ip,port_int);

    //peers_str
    std::stringstream peers_ss(peers_str);
    while(!peers_ss.eof())
    {
        NodeInfo temp_info;
        ip = "";
        port = "";
        getline(peers_ss,ip,':');
        getline(peers_ss,port,',');
        port_int = stoi(port);
        temp_info.SetIPPort(ip,port_int);
        peers_info.push_back(temp_info);
    }

    return 1;
}

PhxAPIServer::~PhxAPIServer()
{
    delete m_poPaxosNode;
}

int PhxAPIServer::makeLogPath(std::string & path)
{
    char sTmp[128] = {0};
    snprintf(sTmp, sizeof(sTmp), "%s/mmlp_paxos_log_%s_%d", phxpaxos_log_path ,local_info.GetIP().c_str(), local_info.GetPort());

    path = std::string(sTmp);

    if (access(path.c_str(), F_OK) == -1)
    {
        if (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
        {
            if(DEBUG_PHXPAXOS_PRINT!=0)
            {
                printf("Create dir fail, path %s\n", path.c_str());
            }

            return -1;
        }
    }

    return 0;
}


int PhxAPIServer::RunPaxos()
{
    Options oOptions;
    int ret = 0;
    // int ret = MakeLogStoragePath(oOptions.sLogStoragePath);
    // if (ret != 0)
    // {
    //     return ret;
    // }

    //this groupcount means run paxos group count.
    //every paxos group is independent, there are no any communicate between any 2 paxos group.
    oOptions.iGroupCount = 1;

    oOptions.oMyNode = local_info;
    oOptions.vecNodeInfoList = peers_info;

    GroupSMInfo oSMInfo;
    oSMInfo.iGroupIdx = 0;
    //one paxos group can have multi state machine.
    oSMInfo.vecSMList.push_back(&api_sm);
    oOptions.vecGroupSMInfoList.push_back(oSMInfo);

    makeLogPath(oOptions.sLogStoragePath);
    if(ret != 0)
    {
        return ret;
    }
    oOptions.eLogLevel = phxpaxos::LogLevel::LogLevel_None;

    ret = Node::RunNode(oOptions, m_poPaxosNode);

    if (ret != 0)
    {
        if(DEBUG_PHXPAXOS_PRINT!=0)
        {
            printf("run paxos fail, ret %d\n", ret);
        }

        return ret;
    }

    if(DEBUG_PHXPAXOS_PRINT!=0)
    {
        printf("run paxos ok\n");
    }

    return 0;
}

int PhxAPIServer::propose(std::string & message,uint64_t & no)
{
    SMCtx oCtx;
    PhxAPISMCtx apiCtx;
    //smid must same to PhxEchoSM.SMID().
    oCtx.m_iSMID = 1;
    oCtx.m_pCtx = (void *)&apiCtx;

    //uint64_t llInstanceID = 0;
    message += "@"+local_info.GetIP()+":"+std::to_string(local_info.GetPort());

    int ret = m_poPaxosNode->Propose(0,message, no, &oCtx);

    if (ret != 0)
    {
        if(DEBUG_PHXPAXOS_PRINT!=0)
        {
            printf("paxos propose fail, ret %d\n", ret);
        }
        return ret;
    }

    if (apiCtx.iExecuteRet != 0)
    {
        if(DEBUG_PHXPAXOS_PRINT!=0)
        {
            printf("api sm excute fail, excuteret %d\n", apiCtx.iExecuteRet);
        }
        return apiCtx.iExecuteRet;
    }

    return 0;
}