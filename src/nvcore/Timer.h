// This code is in the public domain -- castano@gmail.com

#ifndef NV_CORE_TIMER_H
#define NV_CORE_TIMER_H

#include <nvcore/nvcore.h>

#if 1

#include <time.h> //clock

class NVCORE_CLASS Timer
{
public:
	Timer() {}
	
	void start() { m_start = clock(); }
    void stop() { m_stop = clock(); }

	int elapsed() const { return (1000 * (m_stop - m_start)) / CLOCKS_PER_SEC; }
	
private:
	clock_t m_start;
    clock_t m_stop;
};

#else

#define WINDOWS_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>

class NVCORE_CLASS Timer
{
public:
	Timer() {
        // get the tick frequency from the OS
        QueryPerformanceFrequency((LARGE_INTEGER*) &m_frequency);
    }
	
	void start() { QueryPerformanceCounter((LARGE_INTEGER*) &m_start); }
    void stop() { QueryPerformanceCounter((LARGE_INTEGER*) &m_stop); }

	int elapsed() const {
        return (int)1000 * ((double)m_stop.QuadPart - (double)m_start.QuadPart) / (double)m_frequency.QuadPart;
    }
	
private:
    LARGE_INTEGER  m_frequency;
    LARGE_INTEGER  m_start;
    LARGE_INTEGER  m_stop;

};

#endif // 0



#endif // NV_CORE_TIMER_H
