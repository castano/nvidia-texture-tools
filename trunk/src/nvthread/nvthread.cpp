
#include "nvthread.h"

#include "Thread.h"

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>

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
#elif NV_OS_DARWIN || NV_OS_FREEBSD
    int numCPU;
    int mib[4];
    size_t len = sizeof(numCPU); 

    // set the mib for hw.ncpu
    mib[0] = CTL_HW;
    mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;

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

