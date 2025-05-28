# ARM11 has already been included.

# SyncTest
add_executable(SyncTest SyncTest.c)
target_link_libraries(SyncTest PRIVATE gx n3ds11)
make_firm_binary(SyncTest SyncTest_bin)

# RectTest
add_executable(RectTest RectTest.c)
target_link_libraries(RectTest PRIVATE gx n3ds11)
make_firm_binary(RectTest RectTest_bin)