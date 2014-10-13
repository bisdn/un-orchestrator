CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(example)

IF(CMAKE_COMPILER_IS_GNUCXX)
        ADD_DEFINITIONS(-Wall -Werror -Wno-write-strings )# -fbranch-probabilities)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)
IF(CMAKE_COMPILER_IS_GNUCC)
        ADD_DEFINITIONS(-Wall -Werror -Wno-write-strings )# -fbranch-probabilities)
ENDIF(CMAKE_COMPILER_IS_GNUCC)

IF( NOT CMAKE_BUILD_TYPE )
set( CMAKE_BUILD_TYPE Release CACHE STRING
       "Choose the type of build, options are: None Debug Release RelWithDebInfo
MinSizeRel."
       FORCE )
ENDIF( NOT CMAKE_BUILD_TYPE )

OPTION(
	ENABLE_LOG
	"Turn on the print of some information"
	OFF
)
IF(ENABLE_LOG)
	ADD_DEFINITIONS(-DENABLE_LOG)
ENDIF(ENABLE_LOG)

# Set source files
SET(SOURCES
	example.c
	
)



# Create the executable
ADD_EXECUTABLE(
	example
	${SOURCES}
)



LINK_DIRECTORIES(
	/usr/lib/
	/usr/local/lib/
	/usr/x86_64-linux-gnu/
	/usr/lib/x86_64-linux-gnu/
)


TARGET_LINK_LIBRARIES( example
	libpcap.so
)

