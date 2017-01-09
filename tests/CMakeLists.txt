add_executable(test_basic main.c test_basic.c netsim.c)
target_link_libraries(test_basic frul)
add_test(NAME TEST-BASIC COMMAND test_basic)