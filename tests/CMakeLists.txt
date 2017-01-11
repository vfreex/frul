add_executable(test_basic main.c test_basic.c netsim.c test_frul.c)
target_link_libraries(test_basic frul pthread)

add_executable(test_frul test_frul.c)
target_link_libraries(test_frul frul pthread)

add_test(NAME TEST-BASIC COMMAND test_basic test_frul)
