cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(kygx-test-downscale)

add_executable(KYGXTestDownscale Main.c)
target_link_libraries(KYGXTestDownscale PRIVATE kygx)
ctr_create_3dsx(KYGXTestDownscale ROMFS RomFS)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/KYGXTestDownscale.3dsx TYPE BIN)