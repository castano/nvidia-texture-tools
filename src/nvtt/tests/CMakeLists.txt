
ADD_EXECUTABLE(filtertest filtertest.cpp ../tools/cmdline.h)
TARGET_LINK_LIBRARIES(filtertest nvcore nvmath nvimage)

ADD_EXECUTABLE(nvtestsuite stress.cpp)
TARGET_LINK_LIBRARIES(nvtestsuite nvcore nvmath nvimage nvtt)

IF (CUDA_FOUND)
	ADD_EXECUTABLE(driverapitest driverapi.cpp)
	TARGET_LINK_LIBRARIES(driverapitest nvcore nvmath nvimage)
ENDIF (CUDA_FOUND)

ADD_EXECUTABLE(imperativeapi imperativeapi.cpp)
TARGET_LINK_LIBRARIES(imperativeapi nvcore nvmath nvimage nvtt)

INSTALL(TARGETS nvtestsuite DESTINATION bin)
 
#include_directories("/usr/include/ffmpeg/")
#ADD_EXECUTABLE(nvmpegenc tools/mpegenc.cpp tools/cmdline.h)
#TARGET_LINK_LIBRARIES(nvmpegenc nvcore nvmath nvimage avcodec z)
#INSTALL(TARGETS nvmpegenc DESTINATION bin)

