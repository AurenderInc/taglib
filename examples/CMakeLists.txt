include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../taglib
  ${CMAKE_CURRENT_SOURCE_DIR}/../taglib/toolkit
  ${CMAKE_CURRENT_SOURCE_DIR}/../taglib/ape
  ${CMAKE_CURRENT_SOURCE_DIR}/../taglib/mpeg
  ${CMAKE_CURRENT_SOURCE_DIR}/../taglib/mpeg/id3v1
  ${CMAKE_CURRENT_SOURCE_DIR}/../taglib/mpeg/id3v2
  ${CMAKE_CURRENT_SOURCE_DIR}/../bindings/c/
)

if(NOT BUILD_SHARED_LIBS)
  add_definitions(-DTAGLIB_STATIC)
endif()


#JBH ==========================================================================<
if(${JBH_USE_EMBEDDED_UNICODE_ENCODER})
  message("JBH: examples/JBH_USE_EMBEDDED_UNICODE_ENCODER:   " ${JBH_USE_EMBEDDED_UNICODE_ENCODER})
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../taglib/toolkit/jbh) #for charsetdetector.h
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../taglib/toolkit/jbh) #for charsetconverter.h
endif()
#JBH ==========================================================================>


########### next target ###############

add_executable(tagreader tagreader.cpp)
target_link_libraries(tagreader tag ${UCHARDET_LIBRARIES}) #JBH
install(TARGETS tagreader DESTINATION ${BIN_INSTALL_DIR}) #JBH add
install(FILES tagreader.sh t PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE DESTINATION ${BIN_INSTALL_DIR}) #JBH add
if(JBH_BUILD_FOR_GENERAL)
	install(FILES tagreader-general.sh PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE DESTINATION ${BIN_INSTALL_DIR}) #JBH add
endif()

########### next target ###############

add_executable(tagreader_c tagreader_c.c)
target_link_libraries(tagreader_c tag_c ${UCHARDET_LIBRARIES}) #JBH
install(TARGETS tagreader_c DESTINATION ${BIN_INSTALL_DIR}) #JBH add

########### next target ###############

add_executable(tagwriter tagwriter.cpp)
target_link_libraries(tagwriter tag ${UCHARDET_LIBRARIES}) #JBH
install(TARGETS tagwriter DESTINATION ${BIN_INSTALL_DIR}) #JBH add

########### next target ###############

add_executable(framelist framelist.cpp)
target_link_libraries(framelist tag ${UCHARDET_LIBRARIES}) #JBH
install(TARGETS framelist DESTINATION ${BIN_INSTALL_DIR}) #JBH add

########### next target ###############

add_executable(strip-id3v1 strip-id3v1.cpp)
target_link_libraries(strip-id3v1 tag ${UCHARDET_LIBRARIES}) #JBH
install(TARGETS strip-id3v1 DESTINATION ${BIN_INSTALL_DIR}) #JBH add

