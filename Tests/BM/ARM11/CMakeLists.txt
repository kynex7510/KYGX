ctr_bm_enable_arm11()

# TestSync
add_executable(KYGXTestSync TestSync.c)
target_link_libraries(KYGXTestSync PRIVATE kygx)
target_link_options(KYGXTestSync PRIVATE -T ${CTR_BM_TOOLCHAIN_ROOT}/Linker/Default11.ld)
ctr_bm_add_firm_binary(KYGXTestSync_bin KYGXTestSync)

# TestAsync
add_executable(KYGXTestAsync TestAsync.c)
target_link_libraries(KYGXTestAsync PRIVATE kygx)
target_link_options(KYGXTestAsync PRIVATE -T ${CTR_BM_TOOLCHAIN_ROOT}/Linker/Default11.ld)
ctr_bm_add_firm_binary(KYGXTestAsync_bin KYGXTestAsync)

# TestQTM
add_executable(KYGXTestQTM TestQTM.c)
target_link_libraries(KYGXTestQTM PRIVATE kygx)
target_link_options(KYGXTestQTM PRIVATE -T ${CTR_BM_TOOLCHAIN_ROOT}/Linker/Default11.ld)
ctr_bm_add_firm_binary(KYGXTestQTM_bin KYGXTestQTM)