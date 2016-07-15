/*
 *  MiniLog.cpp
 *  MiniLog
 *
 *  Created by Huahua on 16/7/15.
 *  Copyright © 2016年 Huahua Workspace. All rights reserved.
 *
 */

#include <iostream>
#include "MiniLog.hpp"

__MINILOG_NS_BEGIN__

static const char *MiniLogLevelString[] = 
{
	"TRACE",
	"DEBUG",
	"INFO ",
	"WARN ",
	"ERROR"
};


class MiniLog : public IMiniLog
{
public:
	MiniLog() { }
	~MiniLog() { }


	// 通过 IMiniLog 继承
	virtual void SetConfig(const MiniLogConfig config) override
	{
	}

	virtual void SetLogPath(const char * path) override
	{
	}

	virtual bool Start() override
	{
		return false;
	}

	virtual void Stop() override
	{
	}

	virtual bool GetEnable() override
	{
		return true;
	}

	virtual void PushMessage(MiniLogLevel level, const char * file, const int line, const std::string & message) override
	{
		char msg[1024] = { 0 };
		int len = 0;
		char base[] = "%s[%s]";

		if (1) {
			len = snprintf(msg, sizeof(msg), base, msg, "789");
		}
		if (1) {
			char szLine[256] = { 0 };
			snprintf(szLine, sizeof(szLine), "%s:%d", file, line);
			len = snprintf(msg, sizeof(msg), base, msg, szLine);
		}
		if (1) {
			len = snprintf(msg, sizeof(msg), base, msg, MiniLogLevelString[level]);
		}
		len = snprintf(msg, sizeof(msg), base, msg, message.c_str());
		printf("[Length : %d]%s\n", len, msg);
	}

};

IMiniLog * __init__ = nullptr;
IMiniLog * IMiniLog::GetInstance() {
    if (__init__ == nullptr) {
        __init__ = new MiniLog;
    }
    return __init__;
}

__MINILOG_NS_END__
