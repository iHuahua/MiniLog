/*
 *  MiniLog.cpp
 *  MiniLog
 *
 *  Created by Huahua on 16/7/15.
 *  Copyright © 2016年 Huahua Workspace. All rights reserved.
 *
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include "MiniLog.hpp"

#if defined(_WIN32)
#include <Windows.h>
#include <process.h>
#else 
#include <pthread.h>
#endif // _WIN32

/*
 *	Cross platform Macro
 */
#if defined(_WIN32)
#define MLCALLAPI WINAPI
typedef unsigned thread_return_t;
#else
#define MLCALLAPI
typedef void * thread_return_t;
#endif // defined(_WIN32)




__MINILOG_NS_BEGIN__

using std::string;
using std::deque;
using std::vector;
using std::fstream;

class Utility;
class ThreadLaunch;
class MiniLog;
class IMiniLogTarget;
class MiniLogTargetFile;
class MiniLogTargetConsole;
class MiniMessage;


static const char *FormatBase = "%s[%s]";
static const char *FormatCombine = "%s%s";
static const char *MiniLogLevelString[] =
{
	"TRACE",
	"DEBUG",
	"INFO ",
	"WARN ",
	"ERROR"
};
#if defined(_WIN32)
static const short MiniLogLevelColor[] =
{
    0,
    0,
    FOREGROUND_GREEN,
    FOREGROUND_RED | FOREGROUND_GREEN,
    FOREGROUND_RED | FOREGROUND_INTENSITY
};
#else
static const char *MiniLogLevelColor[] =
{
    "\e[0m",
    "\e[0m",
    "\e[32m",       //green
    "\e[33m\e[1m",  //hight yellow
    "\e[31m\e[1m",  //hight red
};
#endif



class Utility
{
public:
    typedef struct DateTime
    {
        uint16_t wYear;
        uint16_t wMonth;
        uint16_t wDayOfWeek;	//!< 0=星期日，1=星期一...
        uint16_t wDay;
        uint16_t wHour;
        uint16_t wMinute;
        uint16_t wSecond;
    } DateTime;
    
    static DateTime GetCurrentTime() {
        DateTime stTime;
#if  defined(WIN32) || defined(WINCE)
        SYSTEMTIME t_time;
        GetLocalTime(&t_time);
        stTime.wYear        = t_time.wYear;
        stTime.wMonth       = t_time.wMonth;
        stTime.wDayOfWeek   = t_time.wDayOfWeek;
        stTime.wDay         = t_time.wDay;
        stTime.wHour        = t_time.wHour;
        stTime.wMinute      = t_time.wMinute;
        stTime.wSecond      = t_time.wSecond;
#else    // for linux
        time_t now;
        struct tm *timenow;
        time(&now);                    //time函数读取现在的时间(国际标准时间非北京时间)，然后传值给now
        timenow = localtime(&now);     //localtime函数把从time取得的时间now换算成你电脑中的时间(就是你设置的地区)
        
        stTime.wYear        = 1900 + timenow->tm_year;
        stTime.wMonth       = 1 + timenow->tm_mon;
        stTime.wDayOfWeek   = timenow->tm_wday;
        stTime.wDay         = timenow->tm_mday;
        stTime.wHour        = timenow->tm_hour;
        stTime.wMinute      = timenow->tm_min;
        stTime.wSecond      = timenow->tm_sec;
#endif    // defined(WIN32) || defined(WINCE)
        return stTime;
    }
    
    static string GetCurrentTimeString() {
        // TODO
		return "2016/07/17 18:57:10.996";
	}
    
    static void Sleep(uint32_t ms) {
        
    }
};

class ThreadLaunch
{
#if defined(_WIN32)
	unsigned m_ThreadID;
#else
    pthread_t m_ThreadID;
#endif
public:
	ThreadLaunch() : m_ThreadID(0) { }
	virtual ~ThreadLaunch() { }
	virtual void Run() = 0;

	bool Start() {
#if defined(_WIN32)
		unsigned tid = _beginthreadex(0, 0, ThreadProc, (void *)this, 0, NULL);
		if (tid == -1 || tid == 0) {
			std::cerr << "[MiniLog] create thread failed\n";
			return false;
		}
		m_ThreadID = tid;
#else
		int ret = pthread_create(&m_ThreadID, NULL, ThreadProc, (void*)this);
		if (ret != 0) {
			std::cerr << "[MiniLog] create thread failed\n";
			return false;
		}
#endif // defined(_WIN32)

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

	static thread_return_t MLCALLAPI ThreadProc(void *pArg) {
		ThreadLaunch *pThread = (ThreadLaunch *)pArg;
		pThread->Run();
		return 0;
	}
private:

};

class MiniMessage
{
public:
	MiniLogLevel m_Level;
	string content;
};

class IMiniLogTarget
{
protected:
    MiniLog *m_MiniLog;
public:
    IMiniLogTarget(MiniLog *miniLog) {
        m_MiniLog = miniLog;
    }
    virtual ~IMiniLogTarget() { }
    virtual bool Start() = 0;
    virtual bool Stop() = 0;
    virtual bool ProcMessage(MiniMessage *) = 0;
};

class MiniLogTargetFile : public IMiniLogTarget
{
    fstream m_FStream;
public:
    MiniLogTargetFile(MiniLog * miniLog) : IMiniLogTarget(miniLog) {
    }
    
    virtual bool Start() override {
        return true;
    }
    
    virtual bool Stop() override {
        return false;
    }
    
    virtual bool ProcMessage(MiniMessage * msg) override {
        return true;
    }
};

class MiniLogTargetConsole : public IMiniLogTarget
{
    
public:
    MiniLogTargetConsole(MiniLog * miniLog) : IMiniLogTarget(miniLog) {
    }
    
    virtual bool Start() override {
        return true;
    }
    
    virtual bool Stop() override {
        return true;
    }
    
    virtual bool ProcMessage(MiniMessage * msg) override;
};

class MiniLog : public IMiniLog, public ThreadLaunch
{
	string m_RootDirectory;
	string m_SubDirectory;

	uint32_t m_Config;
	bool m_LogEnable;
	bool m_Running;

	deque<MiniMessage *> m_Messages;
    vector<IMiniLogTarget *> m_Targets;
public:
    MiniLog() {
        m_LogEnable = true;
        m_Running = false;
        m_Config = MLC_OPT_FILE | MLC_OPT_CONSOLE
        | MLC_FMT_DATETIME | MLC_FMT_LEVEL
        | MLC_FMT_ERRORSTACK | MLC_FMT_COLOR;
	}
	~MiniLog() {
        Release();
    }

	virtual void SetConfig(const MiniLogConfig &config) override {
		m_Config = config;
	}
    uint32_t GetConfig() {
        return m_Config;
    }

	virtual void SetLogPath(const std::string &path) override {
		m_RootDirectory = path;
	}

	virtual bool Start() override {
		if (!m_LogEnable || m_Running) {
			return false;
		}

        if (!InitForStart()) {
            std::cerr << "MiniLog init failed\n";
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
		m_Running = false;
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
        int messageLen = MakeHeader(header, sizeof(header), level, file, line);
		MiniMessage *msg = new MiniMessage;
		
        if (messageLen > 0) {
            messageLen += (int)body.size();
			message = new char[messageLen];
			if (message != NULL) {
                messageLen = snprintf(message, messageLen, FormatCombine, header, body.c_str());
				msg->content = message;
				delete[] message;
			}
		} 
		else {
			msg->content = body;
		}
		msg->m_Level = level;

		return msg;
	}
    
    int MakeHeader(char *header, int headerLen, MiniLogLevel level, const std::string file, int line) {
        int messageLen = 0;

		if (m_Config & MLC_FMT_DATETIME) {
			messageLen = snprintf(header, headerLen, FormatBase, header,
				Utility::GetCurrentTimeString().c_str());
		}
		if (m_Config & MLC_FMT_LEVEL) {
			messageLen = snprintf(header, headerLen, FormatBase, header,
				MiniLogLevelString[level]);
		}
		if (m_Config & MLC_FMT_FILELINE) {
			char szFileLine[256] = { 0 };
			snprintf(szFileLine, sizeof(szFileLine), "%s:%d", file.c_str(), line);
			messageLen = snprintf(header, headerLen, FormatBase, header, szFileLine);
		}
        
        return messageLen;
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
    
    bool InitForStart() {
        ReleaseTargets();
        return InitTargets();
    }
    
    bool InitTargets() {
        bool retval = true;
        if (m_Config & MLC_OPT_FILE) {
            IMiniLogTarget *pTarget = new MiniLogTargetFile(this);
            if (pTarget->Start()) {
                m_Targets.push_back(pTarget);
            }
            else {
                delete pTarget;
                pTarget = NULL;
                retval = false;
            }
        }
        if (m_Config & MLC_OPT_CONSOLE) {
            IMiniLogTarget *pTarget = new MiniLogTargetConsole(this);
            if (pTarget->Start()) {
                m_Targets.push_back(pTarget);
            }
            else {
                delete pTarget;
                pTarget = NULL;
                retval = false;
            }
        }
        return retval;
    }
    
    void ReleaseTargets() {
        while (!m_Targets.empty()) {
            IMiniLogTarget * pTarget = m_Targets.back();
            m_Targets.pop_back();
            delete pTarget;
            pTarget = NULL;
        }
    }
    
    void ReleaseMessages() {
        while (!m_Messages.empty()) {
            MiniMessage * pmsg = m_Messages.front();
            m_Messages.pop_front();
            delete pmsg;
            pmsg = NULL;
        }
    }
    
    void Release() {
        ReleaseMessages();
        ReleaseTargets();
    }

	virtual void Run() override	{
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
                for (size_t tid = 0; tid < m_Targets.size(); ++tid) {
                    m_Targets[tid]->ProcMessage(message);
                }
				DeleteMessage(message);
			}

			if (!m_Running) {
				break;
			}
            
            // TODO
            // sleep (50)
		}
	}
};


bool MiniLogTargetConsole::ProcMessage(MiniMessage * msg) {
    if (m_MiniLog->GetConfig() & MLC_FMT_COLOR) {
        std::cout << MiniLogLevelColor[msg->m_Level];
    }
    
    std::cout << msg->content << std::endl;
    
    if (m_MiniLog->GetConfig() & MLC_FMT_COLOR) {
        std::cout << "\e[0m";
    }

    return true;
}

IMiniLog * __init__ = nullptr;
IMiniLog * IMiniLog::GetInstance() {
    if (__init__ == nullptr) {
        __init__ = new MiniLog;
    }
    return __init__;
}

__MINILOG_NS_END__
