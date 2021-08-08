


add_library(power SHARED Connection.cpp Device.cpp )
target_link_libraries( power pthread )
set_target_properties( power PROPERTIES  LIBRARY_OUTPUT_DIRECTORY ".." )

add_library(weather SHARED Weather.cpp )
target_link_libraries( weather curl )
set_target_properties( weather PROPERTIES  LIBRARY_OUTPUT_DIRECTORY ".." )

add_library(json SHARED JsonParser.cpp )
set_target_properties( json PROPERTIES  LIBRARY_OUTPUT_DIRECTORY ".." )

add_executable(sprinklers main.cpp History.cpp )
target_link_libraries( sprinklers power weather json )
set_target_properties( sprinklers PROPERTIES  RUNTIME_OUTPUT_DIRECTORY ".." )

###############################################
# Webserver
add_executable(webserver webserver.cpp ${MONGOOSE_SRC}/mongoose.c History.cpp )
target_include_directories ( webserver PUBLIC ${MONGOOSE_SRC} )

if( OPENSSL_FOUND )
    message( "Compile & link server with OpenSSL" ) 
    target_compile_definitions(webserver PUBLIC MG_ENABLE_OPENSSL )
    # target_link_libraries( webserver PRIVATE  )
endif()


target_compile_definitions(webserver PUBLIC MG_ENABLE_LOG=0 )
target_link_libraries( webserver power weather json ${OPENSSL_LIBRARIES} )
set_target_properties( webserver PROPERTIES  RUNTIME_OUTPUT_DIRECTORY ".." )
