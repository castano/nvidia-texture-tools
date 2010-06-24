PROJECT(nvtt)

ADD_SUBDIRECTORY(squish)
ADD_SUBDIRECTORY(bc6h)
#ADD_SUBDIRECTORY(bc7)

SET(NVTT_SRCS
	nvtt.h 
	nvtt.cpp
	Context.h
	Context.cpp
	nvtt_wrapper.h
	nvtt_wrapper.cpp
	Compressor.h
	CompressorDXT.h
	CompressorDXT.cpp
	CompressorDX9.h
	CompressorDX9.cpp
	CompressorDX10.h
	CompressorDX10.cpp
	CompressorDX11.h
	CompressorDX11.cpp
	CompressorRGB.h
	CompressorRGB.cpp
	CompressorRGBE.h
	CompressorRGBE.cpp
	QuickCompressDXT.h
	QuickCompressDXT.cpp
	OptimalCompressDXT.h
	OptimalCompressDXT.cpp
	SingleColorLookup.h
	CompressionOptions.h
	CompressionOptions.cpp
	InputOptions.h
	InputOptions.cpp
	OutputOptions.h
	OutputOptions.cpp
	TexImage.h TexImage.cpp
	cuda/CudaUtils.h
	cuda/CudaUtils.cpp
	cuda/CudaMath.h
	cuda/BitmapTable.h
	cuda/CudaCompressorDXT.h
	cuda/CudaCompressorDXT.cpp)

IF (CUDA_FOUND)
	ADD_DEFINITIONS(-DHAVE_CUDA)
	CUDA_COMPILE(CUDA_SRCS cuda/CompressKernel.cu)
	SET(NVTT_SRCS ${NVTT_SRCS} ${CUDA_SRCS})
	SET(LIBS ${LIBS} ${CUDA_LIBRARIES})
	INCLUDE_DIRECTORIES(${CUDA_INCLUDE_DIRS})
ENDIF (CUDA_FOUND)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

ADD_DEFINITIONS(-DNVTT_EXPORTS)

IF(NVTT_SHARED)	
	ADD_LIBRARY(nvtt SHARED ${NVTT_SRCS})
ELSE(NVTT_SHARED)
	ADD_LIBRARY(nvtt ${NVTT_SRCS})
ENDIF(NVTT_SHARED)

TARGET_LINK_LIBRARIES(nvtt ${LIBS} nvcore nvmath nvimage squish bc6h)

INSTALL(TARGETS nvtt 
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib/static)

INSTALL(FILES nvtt.h DESTINATION include/nvtt)


ADD_SUBDIRECTORY(tools)
ADD_SUBDIRECTORY(tests)
