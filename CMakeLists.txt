cmake_minimum_required (VERSION 3.17)

include(FetchContent)

project (power LANGUAGES CXX C)


include(GNUInstallDirs)

set( CMAKE_CXX_FLAGS "-std=c++20" )
add_compile_options( -g )
add_compile_options( -Wfatal-errors )

FetchContent_Declare( 
                mongoose 
                GIT_REPOSITORY  "https://github.com/cesanta/mongoose.git"
                GIT_TAG         "master"  # need 7.3 for cpp support
    )

FetchContent_MakeAvailable( mongoose )  
set( MONGOOSE_SRC ${FETCHCONTENT_BASE_DIR}/mongoose-src )

find_package(OpenSSL)
find_package(jsoncpp REQUIRED)

subdirs(src)
subdirs(tst)
