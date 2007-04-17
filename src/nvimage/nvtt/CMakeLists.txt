PROJECT(nvtt)

ADD_SUBDIRECTORY(squish)

SET(NVTT_SRCS
	nvtt.h 
	CompressDXT.h
	CompressDXT.cpp
	CompressRGB.h
	CompressRGB.cpp
	FastCompressDXT.h
	FastCompressDXT.cpp
	BlockDXT.h
	BlockDXT.cpp
	dxtlib.cpp
	dxtlib_compat.h
	CompressionOptions.h
	CompressionOptions.cpp
	InputOptions.h
	InputOptions.cpp
	OutputOptions.cpp
	cuda/CudaUtils.h
	cuda/CudaUtils.cpp
	cuda/CudaCompressDXT.h
	cuda/CudaCompressDXT.cpp)

IF(CUDA_FOUND)
	ADD_DEFINITIONS(-DHAVE_CUDA)
	WRAP_CUDA(CUDA_SRCS cuda/CompressKernel.cu)
	SET(NVTT_SRCS ${NVTT_SRCS} ${CUDA_SRCS})
	SET(LIBS ${LIBS} ${CUDA_LIBRARY})
	INCLUDE_DIRECTORIES(${CUDA_INCLUDE_PATH})
ENDIF(CUDA_FOUND)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

ADD_DEFINITIONS(-DNVTT_EXPORTS)

IF(NVTT_SHARED)	
	ADD_LIBRARY(nvtt SHARED ${DXT_SRCS})
ELSE(NVTT_SHARED)
	ADD_LIBRARY(nvtt ${NVTT_SRCS})
ENDIF(NVTT_SHARED)

TARGET_LINK_LIBRARIES(nvtt ${LIBS} nvcore nvmath nvimage squish)


# test executables
ADD_EXECUTABLE(nvcompress compress.cpp)
TARGET_LINK_LIBRARIES(nvcompress nvcore nvmath nvimage nvtt)

INSTALL(TARGETS nvcompress DESTINATION bin)

#ADD_EXECUTABLE(nvtextool nvdxt.cpp configdialog.cpp)



