
SET(POSHLIB_SRCS
	posh.c
	posh.h)

ADD_LIBRARY(posh STATIC ${POSHLIB_SRCS})

ADD_EXECUTABLE(archtest tests/arch/archtest.c)
TARGET_LINK_LIBRARIES(archtest posh)

#ADD_EXECUTABLE(linktest tests/linktest/linktest.cpp tests/linktest/testlib.cpp)
#TARGET_LINK_LIBRARIES(linktest posh)

ADD_TEST(POSHTEST archtest)
