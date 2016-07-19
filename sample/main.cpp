//
//  main.cpp
//  MiniLogRun
//
//  Created by Huahua on 16/7/15.
//  Copyright © 2016年 Huahua Workspace. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "MiniLog.hpp"

#if defined(__MINILOG_USING_NAMESPACE__) && (__MINILOG_USING_NAMESPACE__)
using namespace __MINILOG_NS__;
#endif

void log_test() {
    IMiniLog::GetInstance()->Start();

	LOGT("This is a TRACK message %d", 123);
	LOGD("This is a DEBUG message  %s", "fuck u");
	LOGI("This is a INFO  message  %0.03f", 123.0);
	LOGW("This is a WARN  message");
	LOGE("This is a ERROR message");
	IMiniLog::GetInstance()->Stop();
}

void test() {
    time_t t = time( 0 );
    char tmp[64];
    strftime( tmp, sizeof(tmp), "%Y/%m/%d %X %A 本年第%j天 %z", localtime(&t));
    puts( tmp );
}

int main(int argc, const char * argv[]) {
    // insert code here...
//    test();
    log_test();
#ifdef _WIN32
	system("PAUSE");
#endif
    return 0;
}
