#SET(PROG_TARGET scvsmag-dev)

#SET(
#	PROG_SOURCES
#		scvsmag.cpp
#		util.cpp
#		timeline.cpp
#		equations.cpp
#		qualitycontrol.cpp
#		Vs30Mapping.cpp
#		VsSiteCondition.cpp
#		main.cpp
#)

#ADD_DEFINITIONS(-DVSSC)
#SC_ADD_EXECUTABLE(PROG ${PROG_TARGET})
#SC_LINK_LIBRARIES_INTERNAL(${PROG_TARGET} client datamodel_vs)
#SC_INSTALL_INIT(${PROG_TARGET} ../../../trunk/apps/templates/initd.py)

# scvsmaglog
SET(MAIN_PY sceewlog.py)
SET(EEWLOG_TARGET sceewlog)

SET(EEWLOG_FILES sceewlog.py)

SC_ADD_PYTHON_EXECUTABLE(EEWLOG)
SC_INSTALL_INIT(${EEWLOG_TARGET} ../../../../trunk/apps/templates/initd.py)

IF(SC_GLOBAL_PYTHON_WRAPPER)
# UD interface
INSTALL(FILES ud_interface.py
	DESTINATION ${PYTHON_LIBRARY_PATH}
)
SET(UD_DATA config/sc3ml_0.7__quakeml_1.2-RT_eewd.xsl
    config/sc3ml_0.7__shakealert.xsl
)
SC_INSTALL_DATA(UD ${EEWLOG_TARGET})
ENDIF(SC_GLOBAL_PYTHON_WRAPPER)


FILE(GLOB descs "${CMAKE_CURRENT_SOURCE_DIR}/descriptions/*.xml")
INSTALL(FILES ${descs} DESTINATION ${SC3_PACKAGE_APP_DESC_DIR})
