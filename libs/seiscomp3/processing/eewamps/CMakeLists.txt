SET(LIBEEWAMPS_SOURCES
	config.cpp
	router.cpp
	baseprocessor.cpp
	preprocessor.cpp
	processor.cpp
)

SET(LIBEEWAMPS_HEADERS
	config.h
	baseprocessor.h
	processor.h
)

SC_ADD_SUBDIR_SOURCES(LIBEEWAMPS recordfilter)
SC_ADD_SUBDIR_SOURCES(LIBEEWAMPS filter)
SC_ADD_SUBDIR_SOURCES(LIBEEWAMPS processors)

SC_ADD_LIBRARY(LIBEEWAMPS eewamps)
SC_LIB_LINK_LIBRARIES_INTERNAL(eewamps client)
SC_LIB_INSTALL_HEADERS(LIBEEWAMPS)
