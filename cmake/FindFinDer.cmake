# - Find the FinDer includes and library
#

# This module defines
#  FinDer_INCLUDE_DIR, where to find .h, etc.
#  FinDer_LIBRARIES, the libraries to link against to use .
#  FinDer_DEFINITIONS - You should ADD_DEFINITONS(${FinDer_DEFINITIONS}) before compiling code that includes FinDer library files.
#  FinDer_FOUND, If false, do not try to use FinDer.

SET(FinDer_FOUND "NO")

FIND_PATH(FinDer_INCLUDE_DIR finder.h
	/usr/include/
	/usr/local/include/
)

SET(FinDer_NAMES ${FinDer_NAMES} Finder)
FIND_LIBRARY(FinDer_LIBRARY
	NAMES ${FinDer_NAMES}
)

IF(FinDer_LIBRARY AND FinDer_INCLUDE_DIR)
	SET(FinDer_LIBRARIES ${FinDer_LIBRARY} opencv_core opencv_highgui opencv_imgproc gmt)
	SET(FinDer_FOUND "YES")
ENDIF(FinDer_LIBRARY AND FinDer_INCLUDE_DIR)

IF(FinDer_FOUND)
	IF(NOT FinDer_FIND_QUIETLY)
		MESSAGE(STATUS "Found Finder: ${FinDer_LIBRARY}")
	ENDIF(NOT FinDer_FIND_QUIETLY)
ELSE(FinDer_FOUND)
	IF(FinDer_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "Could not find FinDer library")
	ENDIF(FinDer_FIND_REQUIRED)
ENDIF(FinDer_FOUND)
