FIND_PACKAGE(FinDer)

IF(FinDer_FOUND)
	SET(FINDER_TARGET scfinder)
	SET(FINDER_SOURCES main.cpp)

	INCLUDE_DIRECTORIES(${FinDer_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../base/sed-contrib/libs)
	INCLUDE_DIRECTORIES(${FinDer_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/../../../../../base/sed-contrib/libs)
	INCLUDE_DIRECTORIES(${GMT_INCLUDE_DIR})
	ADD_DEFINITIONS("-D_REENTRANT")

	SC_ADD_EXECUTABLE(FINDER ${FINDER_TARGET})
	SC_LINK_LIBRARIES_INTERNAL(${FINDER_TARGET} eewamps datamodel_sm)
	SC_LINK_LIBRARIES(${FINDER_TARGET} ${FinDer_LIBRARIES})

	IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../../trunk/apps/templates/initd.py")
	  SC_INSTALL_INIT(${FINDER_TARGET} ../../../../trunk/apps/templates/initd.py)
	ELSE()
	  SC_INSTALL_INIT(${FINDER_TARGET} ../../../../../base/common/apps/templates/initd.py)
	ENDIF()

	FILE(GLOB descs "${CMAKE_CURRENT_SOURCE_DIR}/descriptions/*.xml")
	INSTALL(FILES ${descs} DESTINATION ${SC3_PACKAGE_APP_DESC_DIR})

	IF(SC_GLOBAL_PYTHON_WRAPPER)
		SET(FD_DATA utils/makeFinDerFiles.py
		utils/makeFinDerPlot_config.py
		utils/makeFinDerPlot.py
		utils/makeFinDerPlot.sh
                utils/makeFinderMask.sh
		)
		SC_INSTALL_DATA(FD FinDer)
	ENDIF(SC_GLOBAL_PYTHON_WRAPPER)

ELSE(FinDer_FOUND)
	MESSAGE(STATUS "libFinder not found, disable compilation of scfinder")
ENDIF(FinDer_FOUND)
