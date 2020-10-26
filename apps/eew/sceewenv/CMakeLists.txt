SET(EEWENV_TARGET sceewenv)

SET(
	EEWENV_SOURCES
		main.cpp
)

INCLUDE_DIRECTORIES(${SC3_PACKAGE_SOURCE_DIR}/../sed/libs)
INCLUDE_DIRECTORIES(${SC3_PACKAGE_BINARY_DIR}/../sed/libs)

SC_ADD_EXECUTABLE(EEWENV ${EEWENV_TARGET})
SC_LINK_LIBRARIES_INTERNAL(${EEWENV_TARGET} eewamps datamodel_vs)

IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../../trunk/apps/templates/initd.py")
  SC_INSTALL_INIT(${EEWENV_TARGET} ../../../../trunk/apps/templates/initd.py)
ELSE()
  SC_INSTALL_INIT(${EEWENV_TARGET} ../../../../../base/common/apps/templates/initd.py)
ENDIF()

FILE(GLOB descs "${CMAKE_CURRENT_SOURCE_DIR}/descriptions/*.xml")
INSTALL(FILES ${descs} DESTINATION ${SC3_PACKAGE_APP_DESC_DIR})
