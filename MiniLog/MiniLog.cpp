/*
 *  MiniLog.cpp
 *  MiniLog
 *
 *  Created by Huahua on 16/7/15.
 *  Copyright © 2016年 Huahua Workspace. All rights reserved.
 *
 */

#include <iostream>
#include <deque>
#include "MiniLog.hpp"

#if defined(_WIN32)
#include <Windows.h>
#include <process.h>
#else 
#endif // _WIN32

/*
	Cross platform Macro
*/
#if defined(_WIN32)
#define MLCALLAPI WINAPI
#else
#define MLCALLAPI
#endif // defined(_WIN32)
#if defined(_WIN32)
typedef unsigned thread_proc_return_type;
#else
typedef void * thread_proc_return_type;
#endif // defined(_WIN32)




__MINILOG_NS_BEGIN__

using std::string;
using std::deque;

class Utility;
class ThreadLaunch;
class MiniLog;
class MiniMessage;


static const char *MiniLogLevelString[] = 
{
	"TRACE",
	"DEBUG",
	"INFO ",
	"WARN ",
	"ERROR"
};

static const char *FormatBase = "%s[%s]";
static const char *FormatCombine = "%s%s";

class Utility {
public:
	static string GetCurrentDateTimeString() {
		return "2015/07/17 18:57:10.996";
	}
};

class ThreadLaunch
{
	unsigned long long m_ThreadID;
public:
	ThreadLaunch() : m_ThreadID(0) { }
	virtual ~ThreadLaunch() { }
	virtual void Run() = 0;

	bool Start() {
		unsigned long long tid = 0;
#if defined(_WIN32)
		tid = _beginthreadex(0, 0, ThreadProc, (void *)this, 0, NULL);
		if (tid == -1 || tid == 0) {
			std::cerr << "[MiniLog] create thread failed\n";
			return false;
		}
#else
		int ret = pthread_create(&tid, NULL, ThreadProc, (void*)this);
		if (ret != 0) {
			std::cerr << "[MiniLog] create thread failed\n";
			return false;
		}
#endif // defined(_WIN32)

		m_ThreadID = tid;
		return true;
	}

	bool Stop() {
#ifdef WIN32
		if (WaitForSingleObject((HANDLE)m_ThreadID, INFINITE) != WAIT_OBJECT_0) {
			return false;
		}
#else
		if (pthread_join(m_ThreadID, NULL) != 0) {
			return false;
		}
#endif
		return true;
	}

	static thread_proc_return_type MLCALLAPI ThreadProc(void *pArg) {
		ThreadLaunch *pThread = (ThreadLaunch *)pArg;
		pThread->Run();
		return 0;
	}
private:

};

class MiniMessage {
public:
	MiniLogLevel m_Level;
	string content;
};

class MiniLog : public IMiniLog, public ThreadLaunch
{
	string m_RootDirectory;
	string m_SubDirectory;

	uint32_t m_Config;
	bool m_LogEnable;
	bool m_Running;

	deque<MiniMessage *> m_Messages;
public:
	MiniLog() {
		m_LogEnable = true;
		m_Running = false;
		m_Config = MLC_OPT_FILE | MLC_OPT_CONSOLE 
			| MLC_FMT_DATETIME | MLC_FMT_LEVEL
			| MLC_FMT_ERRORSTACK | MLC_FMT_COLOR;
	}
	~MiniLog() { }


	virtual void SetConfig(const MiniLogConfig &config) override {
		m_Config = config;
	}

	virtual void SetLogPath(const std::string &path) override {
		m_RootDirectory = path;
	}

	virtual bool Start() override {
		if (!m_LogEnable || m_Running) {
			return false;
		}

		m_Running = ThreadLaunch::Start();
		AddStartupInfo();

		return m_Running;
	}

	virtual void Stop() override {
		if (!m_Running) {
			return;
		}

		AddShutdownInfo();
		ThreadLaunch::Stop();
	}

	virtual void SetEnable(bool enable) override {
		m_LogEnable = enable;
	}

	virtual bool GetEnable() override {
		return m_LogEnable;
	}

	virtual void PushMessage(
		MiniLogLevel level, 
		const std::string & file,
		const int line, 
		const std::string & body) override {
		MiniMessage* msg = MakeMessage(level, file, line, body);
		PushMessage(msg);
	}

private:

	MiniMessage *MakeMessage(
		MiniLogLevel level,
		const std::string & file,
		const int line,
		const std::string & body) {
		
		char header[512] = { 0 };
		char *message = 0;
		int messageLen = 0;
		MiniMessage *msg = new MiniMessage;

		if (m_Config & MLC_FMT_DATETIME) {
			messageLen = snprintf(header, sizeof(header), FormatBase, header, 
				Utility::GetCurrentDateTimeString().c_str());
		}
		if (m_Config & MLC_FMT_LEVEL) {
			messageLen = snprintf(header, sizeof(header), FormatBase, header,
				MiniLogLevelString[level]);
		}
		if (m_Config & MLC_FMT_FILELINE) {
			char szFileLine[256] = { 0 };
			snprintf(szFileLine, sizeof(szFileLine), "%s:%d", file.c_str(), line);
			messageLen = snprintf(header, sizeof(header), FormatBase, header, szFileLine);
		}

		if (messageLen > 0) {
			messageLen = snprintf(0, 0, FormatCombine, header, body.c_str());
			message = new char[messageLen];
			if (message != NULL) {
				messageLen = snprintf(message, messageLen, FormatCombine, header, body.c_str());
				msg->content = message;
				delete message;
			}			
		} 
		else {
			msg->content = body;
		}
		msg->m_Level = level;

		return msg;
	}

	void DeleteMessage(MiniMessage * msg) {
		if (msg != NULL) {
			delete msg;
			msg = NULL;
		}
	}

	void AddStartupInfo() {
		// ----------
		LOGI("------------------------------------------------------------");
		LOGI("--------------------  MiniLog Startup  ---------------------");
	}

	void AddShutdownInfo() {
		LOGI("--------------------  MiniLog Shutdown  --------------------");
		LOGI("------------------------------------------------------------\n\n\n");
	}

	void PushMessage(MiniMessage * msg) {
		// TODO
		// add lock
		m_Messages.push_back(msg);
	}

	bool PopMessage(MiniMessage *&msg) {
		// TODO
		// add lock
		if (m_Messages.empty()) {
			return false;
		}
		msg = m_Messages.front();
		m_Messages.pop_front();

		return true;
	}

	virtual void Run() override
	{
		m_Running = true;
		MiniMessage *message = NULL;

		/*
			output message loop
		*/
		while (true)
		{
			// TODO
			while (PopMessage(message))
			{
				// TODO
				printf(message->content.c_str());
				DeleteMessage(message);
			}

			if (!m_Running) {
				break;
			}
		}
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
