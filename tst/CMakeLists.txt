


add_executable(tst test.cpp )
include_directories( tst "../src" )
target_link_libraries( tst power weather )
set_target_properties( tst PROPERTIES  RUNTIME_OUTPUT_DIRECTORY ".." )
