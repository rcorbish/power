


add_library(power SHARED lib.cpp )
target_link_libraries( power pthread )
set_target_properties( power PROPERTIES  LIBRARY_OUTPUT_DIRECTORY ".." )

add_executable(sprinkler main.cpp )
target_link_libraries( sprinkler power )
set_target_properties( sprinkler PROPERTIES  RUNTIME_OUTPUT_DIRECTORY ".." )
