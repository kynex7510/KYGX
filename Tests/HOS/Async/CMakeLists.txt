cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(kygx-test-async)

add_executable(KYGXTestAsync Main.c)
target_link_libraries(KYGXTestAsync PRIVATE kygx)
ctr_create_3dsx(KYGXTestAsync)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/KYGXTestAsync.3dsx TYPE BIN)