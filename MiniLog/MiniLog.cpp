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


IMiniLogMgr * __init__ = nullptr;
IMiniLogMgr * IMiniLogMgr::GetInstance() {
    if (__init__ == nullptr) {
        __init__ = new IMiniLogMgr;
    }
    return __init__;
}

void IMiniLogMgr::SetConfig(MiniLogConfig)
{
}

bool MiniLog::Init() {
    std::cout << "MiniLog::Init success\n";
    return true;
}


__MINILOG_NS_END__
