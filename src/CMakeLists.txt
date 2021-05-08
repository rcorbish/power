


add_library(power SHARED Connection.cpp Device.cpp )
target_link_libraries( power pthread )
set_target_properties( power PROPERTIES  LIBRARY_OUTPUT_DIRECTORY ".." )

add_library(weather SHARED Weather.cpp )
target_link_libraries( weather curl )
set_target_properties( weather PROPERTIES  LIBRARY_OUTPUT_DIRECTORY ".." )

add_executable(sprinklers main.cpp )
target_link_libraries( sprinklers power weather )
set_target_properties( sprinklers PROPERTIES  RUNTIME_OUTPUT_DIRECTORY ".." )
