add_library(frul SHARED frul.c ../include/common.h)
target_link_libraries(frul crypto)