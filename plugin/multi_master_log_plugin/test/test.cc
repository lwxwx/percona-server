/*
 * @Author: wei
 * @Date: 2020-06-25 19:26:18
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-06-26 08:35:43
 * @Description: file content
 * @FilePath: /multi_master_log_plugin/test/test.cc
 */

#include<iostream>
#include<sstream>
#include<string>

using namespace std;

int main(void)
{
    std::stringstream local_ss("127.0.0.1:12340");
    std::string ip = "";
    std::string port = "";
    getline(local_ss,ip,':');
    getline(local_ss,port);
    int port_int = stoi(port);

    cout << ip << endl;
    cout << port_int << endl;

    std::stringstream peers_ss("127.0.0.1:12340,127.0.0.1:12341");
    while(!peers_ss.eof())
    {
        ip = "";
        port = "";
        getline(peers_ss,ip,':');
        getline(peers_ss,port,',');
        port_int = stoi(port);
        cout << ip << endl;
        cout << port_int << endl;
    }

    return 0;
}