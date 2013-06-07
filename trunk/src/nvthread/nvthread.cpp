// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#include "nvthread.h"

#include "Thread.h"

#if NV_OS_WIN32
#include "Win32.h"
#elif NV_OS_UNIX
#include <sys/types.h>
#include <sys/sysctl.h>
#include <unistd.h>
#elif NV_OS_DARWIN
#import <stdio.h>
#import <string.h>
#import <mach/mach_host.h>
#import <sys/sysctl.h>

//#include <CoreFoundation/CoreFoundation.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#endif

using namespace nv;


// Find the number of cores in the system.
// Based on: http://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine
// @@ Distinguish between logical and physical cores?
uint nv::hardwareThreadCount() {
#if NV_OS_WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo( &sysinfo );
    return sysinfo.dwNumberOfProcessors;
#elif NV_OS_XBOX
    return 3; // or 6?
#elif NV_OS_LINUX // Linux, Solaris, & AIX
    return sysconf(_SC_NPROCESSORS_ONLN);
#elif NV_OS_DARWIN || NV_OS_FREEBSD || NV_OS_OPENBSD
    int numCPU;
    int mib[4];
    size_t len = sizeof(numCPU);

    // set the mib for hw.ncpu
    mib[0] = CTL_HW;

#if NV_OS_OPENBSD
    mib[1] = HW_NCPU;
#else
    mib[1] = HW_AVAILCPU;
#endif

    // get the number of CPUs from the system
    sysctl(mib, 2, &numCPU, &len, NULL, 0);

    if (numCPU < 1) {
        mib[1] = HW_NCPU;
        sysctl( mib, 2, &numCPU, &len, NULL, 0 );

        if (numCPU < 1) {
            return 1; // Assume single core.
        }
    }

    return numCPU;
#else
    return 1; // Assume single core.
#endif
}

