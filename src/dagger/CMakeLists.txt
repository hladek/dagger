include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(dagger2 dagger2.cpp LanguageModel.cpp)
if(LIBUV_FOUND)
    add_definitions(-DLIBUV_FOUND)
    target_link_libraries(dagger2 ${LIBUV_LIBRARIES})
endif(LIBUV_FOUND)
target_link_libraries(dagger2 lemonstring)

add_executable(wordsplit wordsplit.cpp)
target_link_libraries(wordsplit lemonstring)

