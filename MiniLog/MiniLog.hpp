/*
 *  MiniLog.hpp
 *  MiniLog
 *
 *  Created by Huahua on 16/7/15.
 *  Copyright © 2016年 Huahua Workspace. All rights reserved.
 *
 */

#ifndef __MINILOG_HPP__
#define __MINILOG_HPP__


/*
 *  编译宏开关
 *  __MINILOG_USING_NAMESPACE__
 *      是否启用命名空间：0-不启用，1-启用
 *  __MINILOG_NAMESPACE__
 *      命名空间定义，启用命名空间时日志库所在的命名空间，默认 Huahua，可自行修改
 */
#define __MINILOG_USING_NAMESPACE__ 1
#define __MINILOG_NAMESPACE__ Huahua


#if defined(__MINILOG_USING_NAMESPACE__) && (__MINILOG_USING_NAMESPACE__)
#define __MINILOG_NS__ __MINILOG_NAMESPACE__
#define __MINILOG_NS_BEGIN__ namespace __MINILOG_NS__ {
#define __MINILOG_NS_END__ }
#else
#define __MINILOG_NS__
#define __MINILOG_NS_BEGIN__
#define __MINILOG_NS_END__
#endif


#include <string>


__MINILOG_NS_BEGIN__

enum MiniLogLevel {
    MLL_TRACE,
    MLL_DEBUG,
    MLL_INFO,
    MLL_WARN,
    MLL_ERROR
};

enum MiniLogConfig {
    /* output target */
    MLC_OPT_FILE		= 0x0001,
    MLC_OPT_CONSOLE		= 0x0002,
    
    /* message format */
    MLC_FMT_DATETIME	= 0x0100,
	MLC_FMT_LEVEL		= 0x0200,
	MLC_FMT_FILELINE	= 0x0400,
    MLC_FMT_COLOR		= 0x0800,
	/* no supported yet */
    MLC_FMT_ERRORSTACK	= 0x1000
};

/*
 * MiniLog entry.
*/
typedef class IMiniLog {
public:
    static IMiniLog * GetInstance();
	virtual ~IMiniLog() { }
    
	virtual void SetConfig(const MiniLogConfig &config) = 0;
	virtual void SetLogPath(const std::string &path) = 0;
	virtual bool Start() = 0;
	virtual void Stop() = 0;

	virtual void SetEnable(bool) = 0;
	virtual bool GetEnable() = 0;

	virtual void PushMessage(
		MiniLogLevel level,
		const std::string &file,
		const int line,
		const std::string &body
	) = 0;
} IMiniLog;

__MINILOG_NS_END__


/*
void __function(MiniLogLevel level, const char *fmt, ...) {
	do {
		if (!__MINILOG_NS__::IMiniLog::GetInstance()->GetEnable()) break;
		int __ml_length = snprintf(0, 0, fmt, "##__VA_ARGS__") + 1;
		char *__ml_message = new char[__ml_length];
		__ml_length = snprintf(__ml_message, __ml_length, fmt, "##__VA_ARGS__");
		std::string __ml_message_fmt(__ml_message);
		delete __ml_message;
		__MINILOG_NS__::IMiniLog::GetInstance()->PushMessage(
			level, __FILE__, __LINE__, __ml_message_fmt);
	} while (0);
}
*/
#define LOGFMT(level, fmt, ...) \
		do { \
			if (!__MINILOG_NS__::IMiniLog::GetInstance()->GetEnable()) break; \
			int __ml_length = snprintf(0, 0, fmt, ##__VA_ARGS__) + 1; \
			char *__ml_message = new char[__ml_length]; \
			__ml_length = snprintf(__ml_message, __ml_length, fmt, ##__VA_ARGS__); \
			std::string __ml_message_fmt(__ml_message); \
			delete[] __ml_message; \
			__MINILOG_NS__::IMiniLog::GetInstance()->PushMessage( \
			level, __FILE__, __LINE__, __ml_message_fmt); \
		} while (0)

#define LOGT(fmt, ...) LOGFMT(__MINILOG_NS__::MLL_TRACE, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) LOGFMT(__MINILOG_NS__::MLL_DEBUG, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) LOGFMT(__MINILOG_NS__::MLL_INFO,  fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) LOGFMT(__MINILOG_NS__::MLL_WARN,  fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) LOGFMT(__MINILOG_NS__::MLL_ERROR, fmt, ##__VA_ARGS__)

#endif //!__MINILOG_HPP__
