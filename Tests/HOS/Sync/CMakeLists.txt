cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(kygx-test-sync)

add_executable(KYGXTestSync Main.c)
target_link_libraries(KYGXTestSync PRIVATE kygx)
ctr_create_3dsx(KYGXTestSync)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/KYGXTestSync.3dsx TYPE BIN)