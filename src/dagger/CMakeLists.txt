include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(dagger2 dagger2.cpp LanguageModel.cpp)
target_link_libraries(dagger2 lemonstring)
install(TARGETS dagger2 DESTINATION bin)

add_executable(wordsplit wordsplit.cpp)
target_link_libraries(wordsplit lemonstring)
install(TARGETS wordsplit DESTINATION bin)
