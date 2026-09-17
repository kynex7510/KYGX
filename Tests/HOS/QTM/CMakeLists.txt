cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(kygx-test-qtm)

add_executable(KYGXTestQTM Main.c)
target_link_libraries(KYGXTestQTM PRIVATE kygx)
ctr_create_3dsx(KYGXTestQTM)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/KYGXTestQTM.3dsx TYPE BIN)