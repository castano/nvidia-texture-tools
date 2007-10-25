PROJECT(nvtt)

ADD_SUBDIRECTORY(squish)

SET(NVTT_SRCS
	nvtt.h 
	nvtt.cpp
	CompressDXT.h
	CompressDXT.cpp
	CompressRGB.h
	CompressRGB.cpp
	FastCompressDXT.h
	FastCompressDXT.cpp
	QuickCompressDXT.h
	QuickCompressDXT.cpp
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
	SET(LIBS ${LIBS} ${CUDA_LIBRARIES})
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
ADD_EXECUTABLE(nvcompress tools/compress.cpp)
TARGET_LINK_LIBRARIES(nvcompress nvcore nvmath nvimage nvtt)

ADD_EXECUTABLE(nvdecompress tools/decompress.cpp)
TARGET_LINK_LIBRARIES(nvdecompress nvcore nvmath nvimage)

ADD_EXECUTABLE(nvddsinfo tools/ddsinfo.cpp)
TARGET_LINK_LIBRARIES(nvddsinfo nvcore nvmath nvimage)

ADD_EXECUTABLE(nvimgdiff tools/imgdiff.cpp)
TARGET_LINK_LIBRARIES(nvimgdiff nvcore nvmath nvimage)

ADD_EXECUTABLE(nvassemble tools/assemble.cpp)
TARGET_LINK_LIBRARIES(nvassemble nvcore nvmath nvimage)

INSTALL(TARGETS nvcompress nvdecompress nvddsinfo nvimgdiff nvassemble DESTINATION bin)


