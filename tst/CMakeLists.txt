


add_executable(test test.cpp )
include_directories( test "../src" )
target_link_libraries( test power weather )
set_target_properties( test PROPERTIES  RUNTIME_OUTPUT_DIRECTORY ".." )
