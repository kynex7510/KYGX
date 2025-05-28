include(ARM9)
add_executable(Common9 Main.c)
target_link_libraries(Common9 PRIVATE n3ds9)
make_firm_binary(Common9 Common9_bin)