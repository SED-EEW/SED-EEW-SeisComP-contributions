# scof
SET(MAIN_PY scgof.py)
SET(GOF_TARGET scgof)
SET(GOF_FILES scgof.py)

SC_ADD_PYTHON_EXECUTABLE(GOF)

IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../../trunk/apps/templates/initd.py")
SC_INSTALL_INIT(${GOF_TARGET} ../../../../trunk/apps/templates/initd.py)
ELSE()
SC_INSTALL_INIT(${GOF_TARGET} ../../../../../base/common/apps/templates/initd.py)
ENDIF()

FILE(GLOB descs "${CMAKE_CURRENT_SOURCE_DIR}/descriptions/*.xml")
INSTALL(FILES ${descs} DESTINATION ${SC3_PACKAGE_APP_DESC_DIR})