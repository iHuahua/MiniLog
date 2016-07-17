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

static const char *MLLS[] =
{
	"TRACE",
	"DEBUG",
	"INFO ",
	"WARN ",
	"ERROR"
};

void test() {
	int len = snprintf(NULL, 0, "%s", "12345678901234567890") + 1;
	char *msg = new char[len];
	len = snprintf(msg, len, "%s", "12345678901234567890");
	len = snprintf(msg, len, "123");
	printf("[Length : %d][Content : %s]\n", len, msg);
}

void test2() {
	char msg[1024] = { 0 };
	int len = 0;
	char base[] = "%s[%s]";
	char time[] = "2016/07/16 00:51:16.123";
	char file[] = "main.cpp";


	if (1) {
		len = snprintf(msg, sizeof(msg), base, msg, time);
	}
	if (1) {
		len = snprintf(msg, sizeof(msg), base, msg, file);
	}
	if (1) {
		len = snprintf(msg, sizeof(msg), base, msg, MLLS[MLL_ERROR]);
	}
	printf("[Length : %d]%s\n", len, msg);

}

int main(int argc, const char * argv[]) {
    // insert code here...
	IMiniLog::GetInstance()->Start();

	LOGT("TRACK %d", 123);
	LOGD("DEBUG %s", "fuck u");
	LOGI("INFO %0.03f", 123.0);
	LOGW("WARN %d", 123);
	LOGE("ERROR %d", 123);
	IMiniLog::GetInstance()->Stop();

	system("PAUSE");
    return 0;
}
