find_package(benchmark CONFIG REQUIRED)

file(GLOB GTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*test*.cpp")
file(GLOB BENCHMARK_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*bench*.cpp")

foreach(source_file ${GTEST_SOURCES})
    get_filename_component(target_name ${source_file} NAME_WE)
    add_executable(${target_name} ${source_file})
    target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${target_name} PRIVATE GTest::gtest_main GTest::gmock_main)
    gtest_discover_tests(${target_name})
endforeach()

foreach(source_file ${BENCHMARK_SOURCES})
    get_filename_component(target_name ${source_file} NAME_WE)
    add_executable(${target_name} ${source_file})
    target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${target_name} PRIVATE benchmark::benchmark benchmark::benchmark_main)
endforeach()
