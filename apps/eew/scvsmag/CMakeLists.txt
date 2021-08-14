SET(PROG_TARGET scvsmag)

SET(
	PROG_SOURCES
		scvsmag.cpp
		util.cpp
		timeline.cpp
		equations.cpp
		qualitycontrol.cpp
		Vs30Mapping.cpp
		VsSiteCondition.cpp
		main.cpp
)

ADD_DEFINITIONS(-DVSSC)
SC_ADD_EXECUTABLE(PROG ${PROG_TARGET})
SC_LINK_LIBRARIES_INTERNAL(${PROG_TARGET} client datamodel_vs)
IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../../trunk/apps/templates/initd.py")
	SC_INSTALL_INIT(${PROG_TARGET}     ../../../../trunk/apps/templates/initd.py)
ELSE()
	SC_INSTALL_INIT(${PROG_TARGET}     ../../../../../base/common/apps/templates/initd.py)
ENDIF()

FILE(GLOB descs "${CMAKE_CURRENT_SOURCE_DIR}/descriptions/*.xml")
INSTALL(FILES ${descs} DESTINATION ${SC3_PACKAGE_APP_DESC_DIR})
