

add_executable( testcase test.cpp ../src/Logger.cpp )
include_directories( testcase "../src" )
target_link_libraries( testcase power weather jsoncpp_lib )
set_target_properties( testcase PROPERTIES  RUNTIME_OUTPUT_DIRECTORY ".." )
