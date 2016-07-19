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
#else //__linux__
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <semaphore.h>
#endif // _WIN32

#if defined(__APPLE__)
#include <TargetConditionals.h>
#include <dispatch/dispatch.h>
#if !TARGET_OS_IPHONE
#include <libproc.h>
#endif
#endif //__APPLE__

/*
 *	Cross platform Macro
 */
#if defined(_WIN32)
#define MLCALLAPI WINAPI
typedef unsigned thread_return_t;
#else
#define MLCALLAPI
typedef void * thread_return_t;
#endif // _WIN32




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


class DateTime
{
public:
    uint16_t Year;
    uint16_t Month;
    uint16_t DayOfWeek;	//!< 0=星期日，1=星期一...
    uint16_t Day;
    uint16_t Hour;
    uint16_t Minute;
    uint16_t Second;
    uint16_t MilliSecond;
    
public:
    string ToString() {
        char message[24] = { 0 };
        snprintf(message, sizeof(message), "%d/%02d/%02d %02d:%02d:%02d.%03d",
                 Year, Month, Day, Hour, Minute, Second, MilliSecond);
        return message;
    }
};

class Utility
{
public:
    
    static DateTime GetCurrentTime() {
        DateTime stTime;
#if  defined(WIN32)
        SYSTEMTIME t_time;
        GetLocalTime(&t_time);
        stTime.Year        = t_time.wYear;
        stTime.Month       = t_time.wMonth;
        stTime.DayOfWeek   = t_time.wDayOfWeek;
        stTime.Day         = t_time.wDay;
        stTime.Hour        = t_time.wHour;
        stTime.Minute      = t_time.wMinute;
        stTime.Second      = t_time.wSecond;
#else    // for linux
        time_t now;
        struct tm *timenow;
        struct timeval tv;
        time(&now);
        gettimeofday(&tv, NULL);
        timenow = localtime(&now);
        
        stTime.Year        = 1900 + timenow->tm_year;
        stTime.Month       = 1 + timenow->tm_mon;
        stTime.DayOfWeek   = timenow->tm_wday;
        stTime.Day         = timenow->tm_mday;
        stTime.Hour        = timenow->tm_hour;
        stTime.Minute      = timenow->tm_min;
        stTime.Second      = timenow->tm_sec;
        stTime.MilliSecond = tv.tv_usec / 1000;
#endif    // defined(WIN32)
        return stTime;
    }
    
    static void Sleep(uint32_t ms) {
#if defined(_WIN32)
        Sleep(ms);
#else
        usleep(ms * 1000);
#endif
    }
};

class Locker
{
#ifdef _WIN32
    CRITICAL_SECTION _crit;
#else
    pthread_mutex_t  _crit;
#endif
    bool m_Locked;
public:
    Locker() : m_Locked(false) {
#ifdef _WIN32
        InitializeCriticalSection(&_crit);
#else
        //_crit = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&_crit, &attr);
        pthread_mutexattr_destroy(&attr);
#endif
    }
    
    virtual ~Locker() {
#ifdef _WIN32
        DeleteCriticalSection(&_crit);
#else
        pthread_mutex_destroy(&_crit);
#endif
    }
    
    void Lock() {
#ifdef _WIN32
            EnterCriticalSection(&_crit);
#else
            pthread_mutex_lock(&_crit);
#endif
        m_Locked = true;
    }
    
    void Unlock() {
#ifdef _WIN32
        LeaveCriticalSection(&_crit);
#else
        pthread_mutex_unlock(&_crit);
#endif
        m_Locked = false;
    }
    
    bool Locked() {
        return m_Locked;
    }
};

class AutoLocker {
public:
    explicit AutoLocker(Locker & lk) :_lock(lk) { _lock.Lock(); }
    ~AutoLocker() { if (Locked()) Unlock(); }
    void Unlock() { _lock.Unlock(); }
    bool Locked() { return _lock.Locked(); }
private:
    Locker & _lock;
};
#define lock(obj) for (AutoLocker _lock(obj); _lock.Locked(); _lock.Unlock())

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
    DateTime m_DateTime;
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

    Locker m_MessagesLock;
    Locker m_TargetsLock;
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
		MiniMessage *msg = new MiniMessage;
        msg->m_DateTime = Utility::GetCurrentTime();
		
		char header[512] = { 0 };
		char *message = 0;
        int messageLen = MakeHeader(header, sizeof(header), msg->m_DateTime, level, file, line);
		
        if (messageLen > 0) {
            messageLen += (int)body.size() + 1;
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
    
    int MakeHeader(
                   char *header,
                   int headerLen,
                   DateTime time,
                   MiniLogLevel level,
                   const std::string &file,
                   int line) {
        int messageLen = 0;

		if (m_Config & MLC_FMT_DATETIME) {
			messageLen = snprintf(header, headerLen, FormatBase, header,
				time.ToString().c_str());
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

	void DeleteMessage(MiniMessage *& msg) {
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
        lock(m_MessagesLock) {
            m_Messages.push_back(msg);
        }
	}

	bool PopMessage(MiniMessage *&msg) {
        lock(m_MessagesLock) {
            if (m_Messages.empty()) {
                return false;
            }
            msg = m_Messages.front();
            m_Messages.pop_front();
        }
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
        lock(m_TargetsLock) {
            while (!m_Targets.empty()) {
                IMiniLogTarget * pTarget = m_Targets.back();
                m_Targets.pop_back();
                delete pTarget;
                pTarget = NULL;
            }
        }
    }
    
    void ReleaseMessages() {
        lock(m_MessagesLock) {
            while (!m_Messages.empty()) {
                MiniMessage * pmsg = m_Messages.front();
                m_Messages.pop_front();
                delete pmsg;
                pmsg = NULL;
            }
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
            
            Utility::Sleep(100);
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
