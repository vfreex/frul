include_directories(../src)
add_executable(test_temp test_temp.c)
target_link_libraries(test_temp frul pthread)
add_test(NAME TEST-TEMP COMMAND test_temp)