# sceewlog
SET(MAIN_PY sceewlog.py)
SET(EEWLOG_TARGET sceewlog)
SET(EEWLOG_FILES sceewlog.py)

SC_ADD_PYTHON_EXECUTABLE(EEWLOG)


IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../../trunk/apps/templates/initd.py")
  SC_INSTALL_INIT(${EEWLOG_TARGET} ../../../../trunk/apps/templates/initd.py)
ELSE()
  SC_INSTALL_INIT(${EEWLOG_TARGET} ../../../../../base/common/apps/templates/initd.py)
ENDIF()

IF(SC_GLOBAL_PYTHON_WRAPPER)
# UD interface
INSTALL(FILES ud_interface.py headlinealert.py DESTINATION ${PYTHON_LIBRARY_PATH})

SET(UD_DATA config/sc3ml_0.11__quakeml_1.2-RT_eewd.xsl
config/sc3ml_0.11__shakealert.xsl
config/sc3ml_0.11__cap_1.2.xsl
config/closedpolygons.bna
config/world_cities_spanish.csv
config/world_cities_english.csv
)
SC_INSTALL_DATA(UD ${EEWLOG_TARGET})
ENDIF(SC_GLOBAL_PYTHON_WRAPPER)


FILE(GLOB descs "${CMAKE_CURRENT_SOURCE_DIR}/descriptions/*.xml")
INSTALL(FILES ${descs} DESTINATION ${SC3_PACKAGE_APP_DESC_DIR})
