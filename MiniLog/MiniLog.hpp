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
#define __MINILOG_NS_BEGIN__ namespace __MINILOG_NAMESPACE__ {
#define __MINILOG_NS_END__ }
#else
#define __MINILOG_NS_BEGIN__
#define __MINILOG_NS_END__
#endif





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
    MLC_OPT_FILE = 0x1001,
    MLC_OPT_CONSOLE,
    
    /* message format */
    MLC_FMT_DATETIME = 0x2001,
    MLC_FMT_ERRORSTACK,
    MLC_FMT_COLOR
};



class MiniLog {
public:
    static bool Init();
};

class MiniMessage {
    MiniLogLevel m_Level;
    
    
public:
    
};

class IMiniLogMgr {


public:
    static IMiniLogMgr * GetInstance();
    
	void SetConfig(MiniLogConfig config);
};

__MINILOG_NS_END__

#endif //!__MINILOG_HPP__
