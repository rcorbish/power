

add_executable( testcase test.cpp )
include_directories( testcase "../src" )
target_link_libraries( testcase power weather )
set_target_properties( testcase PROPERTIES  RUNTIME_OUTPUT_DIRECTORY ".." )
