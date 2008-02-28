PROJECT(squish)
ENABLE_TESTING()

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

SET(SQUISH_SRCS
#	alpha.cpp
#	alpha.h
#	clusterfit.cpp
#	clusterfit.h
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
#	rangefit.cpp
#	rangefit.h
#	singlecolourfit.cpp
#	singlecolourfit.h
#	singlecolourlookup.inl
#	squish.cpp
#	squish.h
	simd.h
	simd_sse.h
	simd_ve.h)

ADD_LIBRARY(squish STATIC ${SQUISH_SRCS})

# libpng
#FIND_PACKAGE(PNG)

#IF(PNG_FOUND)
#	INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIR})
#	ADD_EXECUTABLE(squishpng extra/squishpng.cpp)
#	TARGET_LINK_LIBRARIES(squishpng squish ${PNG_LIBRARY})
#ENDIF(PNG_FOUND)

##ADD_EXECUTABLE(squishgen extra/squishgen.cpp)

#ADD_EXECUTABLE(squishtest extra/squishtest.cpp)
#TARGET_LINK_LIBRARIES(squishtest squish)

#ADD_TEST(SQUISHTEST squishtest)

