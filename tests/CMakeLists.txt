find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

add_executable(OptionometerTests
    TestTrade.cpp
    TestTradeBuilder.cpp
    TestScorer.cpp
    TestNormalizer.cpp
    TestWeigher.cpp
    TestMathUtils.cpp
    TestSettingsManager.cpp
    TestMarketDataImporter.cpp
)

target_link_libraries(OptionometerTests PRIVATE
    optionometer_lib
    GTest::gtest
    GTest::gtest_main
)

target_include_directories(OptionometerTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

gtest_discover_tests(OptionometerTests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
