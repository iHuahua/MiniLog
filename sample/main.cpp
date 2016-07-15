//
//  main.cpp
//  MiniLogRun
//
//  Created by Huahua on 16/7/15.
//  Copyright © 2016年 Huahua Workspace. All rights reserved.
//

#include <stdlib.h>
#include <iostream>
#include "MiniLog.hpp"

#if defined(__MINILOG_USING_NAMESPACE__) && (__MINILOG_USING_NAMESPACE__)
using namespace __MINILOG_NAMESPACE__;
#endif

int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << MiniLog::Init() << std::endl;
    std::cout << "Hello, World!\n";
	system("PAUSE");
    return 0;
}
