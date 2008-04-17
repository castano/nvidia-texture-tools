// This code is in the public domain -- castanyo@yahoo.es

#include <nvcore/CpuInfo.h>
#include <nvcore/Debug.h>

using namespace nv;

#if NV_OS_WIN32

#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

static bool isWow64()
{
	LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

	BOOL bIsWow64 = FALSE;

	if (NULL != fnIsWow64Process)
	{
		if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
		{
			return false;
		}
	}

	return bIsWow64 == TRUE;
}

#endif // NV_OS_WIN32



uint CpuInfo::processorCount()
{
#if NV_OS_WIN32
	SYSTEM_INFO sysInfo;

	typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

	if (isWow64())
	{
		GetNativeSystemInfo(&sysInfo);
	}
	else
	{
		GetSystemInfo(&sysInfo);
	}

	uint count = (uint)sysInfo.dwNumberOfProcessors;
	nvDebugCheck(count >= 1);

	return count;
#else
	return 1;
#endif
}

uint CpuInfo::coreCount()
{
	return 1;
}

bool CpuInfo::hasMMX()
{
	return false;
}

bool CpuInfo::hasSSE()
{
	return false;
}

bool CpuInfo::hasSSE2()
{
	return false;
}

bool CpuInfo::hasSSE3()
{
	return false;
}



