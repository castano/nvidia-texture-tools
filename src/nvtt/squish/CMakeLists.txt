PROJECT(squish)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

SET(SQUISH_SRCS
	fastclusterfit.cpp
	fastclusterfit.h
	weightedclusterfit.cpp
	weightedclusterfit.h
	colourblock.cpp
	colourblock.h
	colourfit.cpp
	colourfit.h
	colourset.cpp
	colourset.h
	config.h
	maths.cpp
	maths.h
	simd.h
	simd_sse.h
	simd_ve.h)

# Additionnal squish source files which are not included in NVTT by default
IF(SQUISH_FULL_LIBRARY)
	SET(SQUISH_SRCS ${SQUISH_SRCS}
		alpha.h
		alpha.cpp
		clusterfit.h
		clusterfit.cpp
		rangefit.h
		rangefit.cpp
		singlechannelfit.h
		singlechannelfit.cpp
		singlecolourfit.h
		singlecolourfit.cpp
		squish.cpp
	)
ENDIF(SQUISH_FULL)

ADD_LIBRARY(squish STATIC ${SQUISH_SRCS})

IF(NOT WIN32)

	IF("${CMAKE_CXX_COMPILER}" MATCHES "clang(\\+\\+)?$" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		SET(CMAKE_COMPILER_IS_CLANGXX 1)
	ENDIF()

    IF(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
		SET_TARGET_PROPERTIES(squish PROPERTIES COMPILE_FLAGS -fPIC)
    ENDIF(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
ENDIF(NOT WIN32)

IF(INSTALL_SQUISH)
	INSTALL(TARGETS squish
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
	    ARCHIVE DESTINATION lib/static)
ENDIF(INSTALL_SQUISH)
