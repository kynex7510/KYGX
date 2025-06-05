# ARM11 has already been included.

# TestSync
add_executable(KYGXTestSync TestSync.c)
target_link_libraries(KYGXTestSync PRIVATE kygx n3ds11)
make_firm_binary(KYGXTestSync KYGXTestSync_bin)

# TestAsync
add_executable(KYGXTestAsync TestAsync.c)
target_link_libraries(KYGXTestAsync PRIVATE kygx n3ds11)
make_firm_binary(KYGXTestAsync KYGXTestAsync_bin)
