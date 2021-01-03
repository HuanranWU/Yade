IF(${ENABLE_GUI})
  INCLUDE_DIRECTORIES(${QGLVIEWER_INCLUDE_DIR})
  IF(USE_QT5)
    SET(CMAKE_AUTOMOC ON)
    SET(_GLViewer_SOURCE_FILES qt5/GLViewer.cpp;qt5/_GLViewer.cpp;qt5/OpenGLManager.cpp;qt5/GLViewerDisplay.cpp;qt5/GLViewerMouse.cpp)
    ADD_LIBRARY(_GLViewer SHARED ${_GLViewer_SOURCE_FILES})
    SET_TARGET_PROPERTIES(_GLViewer PROPERTIES PREFIX "")
    TARGET_LINK_LIBRARIES(_GLViewer Qt5::Widgets Qt5::Xml Qt5::OpenGL
                                    ${GLUT_LIBRARY} ${OPENGL_LIBRARY}
                                    ${QGLVIEWER_LIBRARIES}
                                    ${Boost_LIBRARIES} ${PYTHON_LIBRARIES})
    IF(GL2PS_FOUND AND ENABLE_GL2PS)
      TARGET_LINK_LIBRARIES(_GLViewer ${GL2PS_LIBRARIES})
    ENDIF(GL2PS_FOUND AND ENABLE_GL2PS)
    INSTALL(TARGETS _GLViewer DESTINATION ${YADE_PY_PATH}/yade/qt)
    
    FILE(GLOB filesPYQT "${CMAKE_CURRENT_SOURCE_DIR}/qt5/*.py")
    INSTALL(FILES ${filesPYQT} DESTINATION ${YADE_PY_PATH}/yade/qt)
    
    EXECUTE_PROCESS(
          COMMAND "pyrcc5" "-o" "${CMAKE_BINARY_DIR}/img_rc.py" "img.qrc"
          WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/qt5
          RESULT_VARIABLE rv
          )
    
    EXECUTE_PROCESS(
          COMMAND "pyuic5" "--from-imports" "-o" "${CMAKE_BINARY_DIR}/ui_controller.py" "controller.ui"
          WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/qt5
          RESULT_VARIABLE rv
          )
    INSTALL(FILES ${CMAKE_BINARY_DIR}/img_rc.py ${CMAKE_BINARY_DIR}/ui_controller.py  DESTINATION ${YADE_PY_PATH}/yade/qt)
    TARGET_LINK_LIBRARIES(_GLViewer Qt5::Widgets Qt5::Xml Qt5::OpenGL
                                    ${GLUT_LIBRARY} ${OPENGL_LIBRARY}
                                    ${QGLVIEWER_LIBRARIES}
                                    ${Boost_LIBRARIES} ${PYTHON_LIBRARIES})
    IF(GL2PS_FOUND AND ENABLE_GL2PS)
      TARGET_LINK_LIBRARIES(_GLViewer ${GL2PS_LIBRARIES})
    ENDIF(GL2PS_FOUND AND ENABLE_GL2PS)
    INSTALL(TARGETS _GLViewer DESTINATION ${YADE_PY_PATH}/yade/qt)
    
    FILE(GLOB filesPYQT "${CMAKE_CURRENT_SOURCE_DIR}/qt5/*.py")
    INSTALL(FILES ${filesPYQT} DESTINATION ${YADE_PY_PATH}/yade/qt)
    
    EXECUTE_PROCESS(
          COMMAND "pyrcc5" "-o" "${CMAKE_BINARY_DIR}/img_rc.py" "img.qrc"
          WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/qt5
          RESULT_VARIABLE rv
          )
    
    EXECUTE_PROCESS(
          COMMAND "pyuic5"  "--from-imports" "-o" "${CMAKE_BINARY_DIR}/ui_controller.py" "controller.ui"
          WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/qt5
          RESULT_VARIABLE rv
          )
    INSTALL(FILES ${CMAKE_BINARY_DIR}/img_rc.py ${CMAKE_BINARY_DIR}/ui_controller.py  DESTINATION ${YADE_PY_PATH}/yade/qt)
  ELSE(USE_QT5)
    MESSAGE(FATAL_ERROR "QT4 is no longer supported")
  ENDIF(USE_QT5)
ENDIF(${ENABLE_GUI})
