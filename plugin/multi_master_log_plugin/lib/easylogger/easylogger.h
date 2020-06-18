/*
 * @Author: wei
 * @Date: 2020-06-16 10:26:05
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-06-16 15:12:57
 * @Description: file content
 * @FilePath: /multi-master-tool/easy_logger/include/easylogger.h
 */
#ifndef EASY_LOGGER_HEADER
#define EASY_LOGGER_HEADER

#include <cstddef>
#include<iostream>
#include <memory>
#include<ostream>
#include<string>
#include<sstream>

#include<map>

#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"

class EasyLogger
{
	public:

	enum LOG_LEVEL
    {
		debug,
		info,
		warn,
		error,
	};

	//EasyLogger()

	static std::string debug_string(std::string file_name,unsigned int line,std::string function_name,std::string action,std::string result)
	{
		std::string res = "{";
		res += "[@file # " + file_name +"],[@line # " + std::to_string(line) + "],[@function # "+ function_name +"],[@action # "+ action + "],[@result # " + result +"]";
		res += "}";
		return res;
	}

	static void logger_record(std::string log_file_name,std::string name,std::string log_value,int m_level,bool will_flush = false)
	{
		std::shared_ptr<spdlog::logger> async_log;
		async_log = spdlog::get(name);
		if(async_log == NULL)
		{
			async_log = spdlog::basic_logger_mt<spdlog::async_factory>(name,log_file_name);
		}
//		std::cout << log_value << std::endl;
		switch (m_level)
        {
        case LOG_LEVEL::info:
			spdlog::set_level(spdlog::level::info);
            async_log->info(log_value);
            break;
        case LOG_LEVEL::debug:
			spdlog::set_level(spdlog::level::debug);
            async_log->debug(log_value);
            break;
        case LOG_LEVEL::warn:
            async_log->warn(log_value);
            break;
        case LOG_LEVEL::error:
            async_log->error(log_value);
            break;
        default:
			std::cout << "Unkown Log Level" << std::endl;
            break;
        }

		if(will_flush)
		{
			async_log->flush();
		}
	}

	//~EasyLogger()
	private:
	EasyLogger();
	~EasyLogger();
};

#define EasyStringLog(log_file,name,str,level) \
	EasyLogger::logger_record(log_file,name,str,level)

#define EasyStringLog_ForceFlush(log_file,name,str,level) \
	EasyLogger::logger_record(log_file,name,str,level,true)

#define EasyDebugLog(f,n,l,action,res) \
	EasyStringLog(f, n,EasyLogger::debug_string(__FILE__, __LINE__ , __FUNCTION__,action,res) , l)

#define EasyDebugLog_ForceFlush(f,n,l,file_name,line,action,res) \
	EasyStringLog_ForceFlush(f, n,EasyLogger::debug_string(__FILE__, __LINE__ , __FUNCTION__,action,res) , l)

#endif // !EASY_LOGGER_HEADER
