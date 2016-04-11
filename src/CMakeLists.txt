project(lemonstore)
cmake_minimum_required(VERSION 2.6)
set(CMAKE_CXX_FLAGS "-Wall")
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
find_package(LibUV)

# Add a top-level "tags" target which includes all files in both
# the build and source versions of src/*.
# ...but only make it a dependency of the project if the ctags program
# is available, else it will fail to build on Windows.
find_program(CTAGS_PATH ctags)
if(CTAGS_PATH)
    message(STATUS "Found ctags: ${CTAGS_PATH}")
    set_source_files_properties(tags PROPERTIES GENERATED true)
    add_custom_target(tags
    COMMAND ctags -R --c++-kinds=+p --fields=+iaS --extra=+q 
        ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

endif(CTAGS_PATH)

find_program(CSCOPE_PATH cscope)
if(CSCOPE_PATH)
    message(STATUS "Found ctags: ${CSCOPE_PATH}")
    set_source_files_properties(cscope.out PROPERTIES GENERATED true)
    add_custom_target(cscope
        COMMAND find  ${CMAKE_CURRENT_SOURCE_DIR} -type f -regex ".*[phc]$" > cscope.files
        COMMAND cscope -b -q -k
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif(CSCOPE_PATH)

add_subdirectory(dagger)
