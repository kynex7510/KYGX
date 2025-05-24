cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(gx-tests)

# RectTest

add_executable(gx-test-rect RectTest.c)
target_link_libraries(gx-test-rect PRIVATE gx)
ctr_create_3dsx(gx-test-rect)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/gx-test-rect.3dsx TYPE BIN)

# SyncTest

add_executable(gx-test-sync SyncTest.c)
target_link_libraries(gx-test-sync PRIVATE gx)
ctr_create_3dsx(gx-test-sync)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/gx-test-sync.3dsx TYPE BIN)