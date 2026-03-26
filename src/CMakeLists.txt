set(LIB_SOURCES
    models/Trade.cpp
    core/MathUtils.cpp
    core/TradeBuilder.cpp
    core/Scorer.cpp
    core/StrategyOptimizerScorer.cpp
    core/BullPutScorer.cpp
    core/Normalizer.cpp
    core/Weigher.cpp
    core/BlackScholes.cpp
    services/MarketDataImporter.cpp
    services/ScreenerService.cpp
    services/SettingsManager.cpp
    ui/DarkTheme.cpp
    ui/MainWindow.cpp
    ui/ScreenerPanel.cpp
    ui/ResultsTab.cpp
    ui/ResultsTableModel.cpp
    ui/ResultsSortFilterProxy.cpp
    ui/ColorDelegate.cpp
    ui/DteCalendar.cpp
    ui/PlChartWidget.cpp
    ui/ChartWindow.cpp
    ui/SettingsDialog.cpp
    workers/ScreenerWorker.cpp
)

set(LIB_HEADERS
    models/Option.h
    models/OptionChain.h
    models/Trade.h
    models/ScoredTrade.h
    models/ScoredBullPut.h
    models/Enums.h
    core/MathUtils.h
    core/TradeBuilder.h
    core/Scorer.h
    core/StrategyOptimizerScorer.h
    core/BullPutScorer.h
    core/Normalizer.h
    core/Weigher.h
    core/BlackScholes.h
    services/MarketDataImporter.h
    services/ScreenerService.h
    services/SettingsManager.h
    ui/DarkTheme.h
    ui/MainWindow.h
    ui/ScreenerPanel.h
    ui/ResultsTab.h
    ui/ResultsTableModel.h
    ui/ResultsSortFilterProxy.h
    ui/ColorDelegate.h
    ui/DteCalendar.h
    ui/PlChartWidget.h
    ui/ChartWindow.h
    ui/SettingsDialog.h
    workers/ScreenerWorker.h
)

add_library(optionometer_lib STATIC ${LIB_SOURCES} ${LIB_HEADERS})
target_include_directories(optionometer_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(optionometer_lib PUBLIC
    Qt6::Widgets
    Qt6::Network
    nlohmann_json::nlohmann_json
)

add_executable(Optionometer WIN32 main.cpp)
target_link_libraries(Optionometer PRIVATE optionometer_lib)
