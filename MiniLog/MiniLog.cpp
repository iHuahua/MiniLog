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
#include <thread>
#include "MiniLog.hpp"

#if defined(_WIN32)
#include <Windows.h>
#else //__linux__
#include <sys/time.h>
#endif // _WIN32



__MINILOG_NS_BEGIN__

using std::string;
using std::deque;
using std::vector;
using std::fstream;
using std::thread;
using std::mutex;

class Utility;
class MiniLog;
class Semaphore;
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
static const char *PathSeparator = "\\";
#else
static const char *MiniLogLevelColor[] =
{
    "\e[0m",
    "\e[0m",
    "\e[32m",       //green
    "\e[33m\e[1m",  //hight yellow
    "\e[31m\e[1m",  //hight red
};
static const char *PathSeparator = "/";
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
        return string(message);
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
        struct tm *tmnow;
        struct timeval tv;
        time(&now);
        gettimeofday(&tv, NULL);
        tmnow = localtime(&now);

        stTime.Year        = 1900 + tmnow->tm_year;
        stTime.Month       = 1 + tmnow->tm_mon;
        stTime.DayOfWeek   = tmnow->tm_wday;
        stTime.Day         = tmnow->tm_mday;
        stTime.Hour        = tmnow->tm_hour;
        stTime.Minute      = tmnow->tm_min;
        stTime.Second      = tmnow->tm_sec;
        stTime.MilliSecond = tv.tv_usec / 1000;
#endif    // defined(WIN32)
        return stTime;
    }

    static void Sleep(uint32_t ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
};

class Semaphore {
public:
    Semaphore (int count_ = 0)
            : count(count_) {}

    inline void notify() {
        std::unique_lock<std::mutex> lock(mtx);
        count++;
        cv.notify_one();
    }

    inline void wait() {
        std::unique_lock<std::mutex> lock(mtx);

        while(count == 0){
            cv.wait(lock);
        }
        count--;
    }

private:
    std::mutex mtx;
    std::condition_variable cv;
    int count;
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
    uint32_t GetConfig();
};

class MiniLogTargetFile : public IMiniLogTarget
{
    fstream m_FStream;
public:
    MiniLogTargetFile(MiniLog * miniLog) : IMiniLogTarget(miniLog) {
    }

    virtual bool Start() override {
        if (m_FStream.is_open())
            return true;

        m_FStream.open("test.log", std::ios::out | std::ios::app);

        return m_FStream.good();
    }

    virtual bool Stop() override {
        if (!m_FStream.is_open())
            return false;

        m_FStream.flush();
        m_FStream.close();
        return true;
    }

    virtual bool ProcMessage(MiniMessage * msg) override {
        m_FStream << msg->content << std::endl;
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

    virtual bool ProcMessage(MiniMessage * msg) override {
        if (EnableColorConsole()) {
            std::cout << MiniLogLevelColor[msg->m_Level];
        }

        std::cout << msg->content << std::endl;

        if (EnableColorConsole()) {
            std::cout << "\e[0m";
        }

        return true;
    }

    bool EnableColorConsole() {
        return ((GetConfig() & MLC_FMT_COLOR) != 0);
    }
};

class MiniLog : public IMiniLog
{
	string m_RootDirectory;
	string m_SubDirectory;

	uint32_t m_Config;
	bool m_LogEnable;
	bool m_Running;

    mutex m_MessagesMutex;
    mutex m_TargetsMutex;
	deque<MiniMessage *> m_Messages;
    vector<IMiniLogTarget *> m_Targets;
    thread m_Thread;
    Semaphore m_ThreadSemaphore;
public:
    MiniLog() {
        m_LogEnable = true;
        m_Running = false;
        m_Config = MLC_OPT_FILE | MLC_OPT_CONSOLE
        | MLC_FMT_DATETIME | MLC_FMT_LEVEL
        | MLC_FMT_ERRORSTACK | MLC_FMT_COLOR | MLC_FMT_FILELINE;
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

        m_Thread = thread(std::mem_fn(&MiniLog::Run), this);
        m_ThreadSemaphore.wait();

		AddStartupInfo();

		return m_Running;
	}

	virtual void Stop() override {
		if (!m_Running) {
			return;
		}

		AddShutdownInfo();
		m_Running = false;
        m_Thread.join();
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

    virtual void PushMiniMessage(const std::string & message) override {
        MiniMessage* msg = new MiniMessage;
        msg->content = message;
        msg->m_Level = MLL_INFO;
        PushMessage(msg);
    }

private:
    bool EnableFileLine() {
        return ((m_Config & MLC_FMT_FILELINE) != 0);
    }

	MiniMessage *MakeMessage(
		MiniLogLevel level,
		const std::string & file,
		const int line,
		const std::string & body) {
		MiniMessage *msg = new MiniMessage;
        msg->m_DateTime = Utility::GetCurrentTime();

		char header[64] = { 0 };
        char tail[512] = { 0 };
		char *message = 0;
        int tailLen = 0;
        int headerLen = MakeHeader(header, sizeof(header), msg->m_DateTime, level);

        if (EnableFileLine()) {
            size_t subbeg = file.rfind(PathSeparator) + 1;
            tailLen = snprintf(tail, sizeof(tail), "[%s:%d]",
                file.substr(subbeg, file.length() - subbeg).c_str(), line);
		}

        if (tailLen > 0 || headerLen > 0) {
            int messageLen = (int)body.size() + headerLen + tailLen + 1;
            message = new char[messageLen];
            if (message != NULL) {
                if (headerLen > 0)
                    snprintf(message, messageLen, FormatCombine, header, body.c_str());
                if (tailLen > 0)
                    strcat(message, tail);
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
        MiniLogLevel level) {
        int messageLen = 0;
        char *tmpheader = new char[headerLen];
        memset(tmpheader, 0, headerLen);

		if (m_Config & MLC_FMT_DATETIME) {
            strncpy(tmpheader, header, headerLen);
			messageLen = snprintf(header, headerLen, FormatBase, tmpheader,
				time.ToString().c_str());
		}
		if (m_Config & MLC_FMT_LEVEL) {
            strncpy(tmpheader, header, headerLen);
			messageLen = snprintf(header, headerLen, FormatBase, tmpheader,
				MiniLogLevelString[level]);
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
		LOGI("------------------------------------------------------------");
		LOGI("--------------------  MiniLog Startup  ---------------------");
	}

	void AddShutdownInfo() {
		LOGI("--------------------  MiniLog Shutdown  --------------------");
		LOGI("------------------------------------------------------------");
        PushMiniMessage("\n\n\n");
	}

	void PushMessage(MiniMessage * msg) {
        m_MessagesMutex.lock();
        m_Messages.push_back(msg);
        m_MessagesMutex.unlock();
	}

	bool PopMessage(MiniMessage *&msg) {
        m_MessagesMutex.lock();
        if (m_Messages.empty()) {
            m_MessagesMutex.unlock();
            return false;
        }
        msg = m_Messages.front();
        m_Messages.pop_front();
        m_MessagesMutex.unlock();
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
        m_TargetsMutex.lock();
        {
            while (!m_Targets.empty()) {
                IMiniLogTarget * pTarget = m_Targets.back();
                m_Targets.pop_back();
                pTarget->Stop();
                delete pTarget;
                pTarget = NULL;
            }
        }
        m_TargetsMutex.unlock();
    }

    void ReleaseMessages() {
        m_MessagesMutex.lock();
        {
            while (!m_Messages.empty()) {
                MiniMessage * pmsg = m_Messages.front();
                m_Messages.pop_front();
                delete pmsg;
                pmsg = NULL;
            }
        }
        m_MessagesMutex.unlock();
    }

    void Release() {
        ReleaseMessages();
        ReleaseTargets();
    }

	virtual void Run() {
		m_Running = true;
        m_ThreadSemaphore.notify();
		MiniMessage *message = NULL;

		/*
			output message loop
		*/
		while (true)
		{
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


uint32_t IMiniLogTarget::GetConfig() {
    return m_MiniLog->GetConfig();
}

IMiniLog * __init__ = nullptr;
IMiniLog * IMiniLog::GetInstance() {
    if (__init__ == nullptr) {
        __init__ = new MiniLog;
    }
    return __init__;
}

__MINILOG_NS_END__
