PROJECT(nvthreads)

SET(THREADS_SRCS
	nvthread.h nvthread.cpp
	Atomic.h
	Event.h Event.cpp
	Mutex.h Mutex.cpp
	ParallelFor.h ParallelFor.cpp
	Thread.h Thread.cpp
	ThreadPool.h ThreadPool.cpp)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# targets
ADD_DEFINITIONS(-DNVTHREADS_EXPORTS)

IF(NVTHREADS_SHARED)
	ADD_LIBRARY(nvthreads SHARED ${THREADS_SRCS})
ELSE(NVTHREADS_SHARED)
	ADD_LIBRARY(nvthreads ${THREADS_SRCS})
ENDIF(NVTHREADS_SHARED)

TARGET_LINK_LIBRARIES(nvthreads ${LIBS} nvcore)

INSTALL(TARGETS nvthreads
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib/static)
