# ARM11 has already been included.

# TestSync
add_executable(KYGXTestSync TestSync.c)
target_link_libraries(KYGXTestSync PRIVATE kygx)
bm_target_linker_file(KYGXTestSync ${CTR_BM_TOOLCHAIN_ROOT}/Linker/Default11.ld)
add_firm_binary(KYGXTestSync_bin KYGXTestSync)

# TestAsync
add_executable(KYGXTestAsync TestAsync.c)
target_link_libraries(KYGXTestAsync PRIVATE kygx)
bm_target_linker_file(KYGXTestAsync ${CTR_BM_TOOLCHAIN_ROOT}/Linker/Default11.ld)
add_firm_binary(KYGXTestAsync_bin KYGXTestAsync)
