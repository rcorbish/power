
add_executable(power main.cpp )
target_link_libraries( power glog pthread )

set_target_properties( power PROPERTIES  RUNTIME_OUTPUT_DIRECTORY ".." )
