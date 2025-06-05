cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(kygx-tests)

# TestSync

add_executable(KYGXTestSync TestSync.c)
target_link_libraries(KYGXTestSync PRIVATE kygx)
ctr_create_3dsx(KYGXTestSync)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/KYGXTestSync.3dsx TYPE BIN)

# TestAsync

add_executable(KYGXTestAsync TestAsync.c)
target_link_libraries(KYGXTestAsync PRIVATE kygx)
ctr_create_3dsx(KYGXTestAsync)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/KYGXTestAsync.3dsx TYPE BIN)

# TestQTM

add_executable(KYGXTestQTM TestQTM.c)
target_link_libraries(KYGXTestQTM PRIVATE kygx)
ctr_create_3dsx(KYGXTestQTM)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/KYGXTestQTM.3dsx TYPE BIN)