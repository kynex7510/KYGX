cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(gx-tests)

# RectTest

add_executable(GXTestRect RectTest.c)
target_link_libraries(GXTestRect PRIVATE gx)
ctr_create_3dsx(GXTestRect)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/GXTestRect.3dsx TYPE BIN)

# SyncTest

add_executable(GXTestSync SyncTest.c)
target_link_libraries(GXTestSync PRIVATE gx)
ctr_create_3dsx(GXTestSync)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/GXTestSync.3dsx TYPE BIN)

# QTMTest

add_executable(GXTestQTM QTMTest.c)
target_link_libraries(GXTestQTM PRIVATE gx)
ctr_create_3dsx(GXTestQTM)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/GXTestQTM.3dsx TYPE BIN)