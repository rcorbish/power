
cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)

add_library(power SHARED Connection.cpp Device.cpp )
target_link_libraries( power pthread )
set_target_properties( power PROPERTIES  LIBRARY_OUTPUT_DIRECTORY ".." )

add_library(weather SHARED Weather.cpp )
target_link_libraries( weather curl jsoncpp_lib )
set_target_properties( weather PROPERTIES  LIBRARY_OUTPUT_DIRECTORY ".." )

add_executable(sprinklers main.cpp History.cpp Options.cpp)
target_link_libraries( sprinklers power weather )
set_target_properties( sprinklers PROPERTIES  RUNTIME_OUTPUT_DIRECTORY ".." )

add_executable(readweather readWeather.cpp Options.cpp )
target_link_libraries( readweather weather )
set_target_properties( readweather PROPERTIES  RUNTIME_OUTPUT_DIRECTORY ".." )

###############################################
# Webserver
add_executable(webserver webserver.cpp ${MONGOOSE_SRC}/mongoose.c History.cpp Options.cpp)
target_include_directories ( webserver PUBLIC ${MONGOOSE_SRC} )

if( OPENSSL_FOUND )
    message( "Compile & link server with OpenSSL" ) 
    target_compile_definitions(webserver PUBLIC MG_ENABLE_OPENSSL )
    target_compile_definitions(webserver PUBLIC MG_TLS=MG_TLS_OPENSSL )
    # target_link_libraries( webserver PRIVATE  )
endif()


target_compile_definitions(webserver PUBLIC MG_ENABLE_LOG=0 )
target_link_libraries( webserver power weather jsoncpp_lib ${OPENSSL_LIBRARIES} )
set_target_properties( webserver PROPERTIES  RUNTIME_OUTPUT_DIRECTORY ".." )
