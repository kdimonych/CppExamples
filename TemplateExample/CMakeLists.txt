# CppExamples
#
# Created by Dmytro Krasnianskyi on 22.11.20.

cmake_minimum_required(VERSION 3.0)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# If necessary, use the RELATIVE flag, otherwise each source file may be listed 
# with full pathname. RELATIVE may makes it easier to extract an executable name
# automatically.
# file( GLOB RELATIVE EXAMPLE_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp )
file( GLOB EXAMPLE_SOURCES_FULL  *.cpp )
file( RELATIVE_PATH EXAMPLE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR} ${EXAMPLE_SOURCES_FULL})
message( STATUS "Start creating examples")

foreach( examplefile IN ITEMS ${EXAMPLE_SOURCES} )
    # I used a simple string replace, to cut off .cpp.
    string( REPLACE ".cpp" "" example ${examplefile} )
    message( STATUS "Create example: " ${example} " from " ${examplefile} )
    add_executable( ${example} ${examplefile} )
    #${example}
    # Make sure YourLib is linked to each app
    #target_link_libraries( ${example} YourLib )
endforeach()

