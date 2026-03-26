# Optionometer

A C++ desktop application for analyzing and screening options trading strategies. Optionometer integrates with the [Market Data API](https://www.marketdata.app/) to pull real-time option chain data, evaluates trades based on probability and profitability metrics, and displays scored results in an interactive GUI.

## Features

- **Strategy Optimizer** — screens and scores multi-leg trades (2/3/4-leg spreads, iron condors)
- **Bull Put Spread Screener** — filters bull put spreads by configurable criteria
- **Bull Put Spread Optimizer** — optimizes contract count for bull put positions
- Probability-of-profit calculations using implied volatility and normal distribution CDF
- Weighted, normalized scoring across multiple metrics (annual return, delta, max profit/loss, etc.)
- Dark-themed Qt6 GUI with sortable/filterable results tables
- Non-blocking screener execution via worker threads

## Architecture

```
src/
├── models/       Data structures (Option, OptionChain, Trade, ScoredTrade)
├── core/         Algorithms — scoring, normalization, weighting, trade building, math utilities
├── services/     Market data fetching (HTTP/JSON), screener orchestration, settings persistence
├── ui/           Qt6 widgets — main window, screener panel, results table, settings dialog, dark theme
├── workers/      Threaded screener worker for non-blocking operation
└── main.cpp      Application entry point
```

**Layer summary:**

| Layer | Responsibility | Key classes |
|-------|---------------|-------------|
| Models | Data representation | `Trade`, `Option`, `OptionChain`, `ScoredTrade` |
| Core | Business logic | `TradeBuilder`, `Scorer`, `BullPutScorer`, `Normalizer`, `Weigher`, `MathUtils` |
| Services | External I/O & orchestration | `MarketDataImporter`, `ScreenerService`, `SettingsManager` |
| UI | Presentation | `MainWindow`, `ScreenerPanel`, `ResultsTab`, `ResultsTableModel` |
| Workers | Concurrency | `ScreenerWorker` |

### Dependencies

- **Qt 6** (Widgets, Network)
- **nlohmann/json** — JSON serialization
- **Google Test** — unit testing
- **vcpkg** — package management

### Build system

- **CMake 3.21+** with C++20
- **MSVC 2022** (cl.exe) via Ninja generator
- Presets defined in `CMakePresets.json`: `x64-debug` and `x64-release`

## Building

```bash
# Configure
cmake --preset x64-debug

# Build
cmake --build out/build/x64-debug
```

The executable is output to `out/build/x64-debug/src/Optionometer.exe`.

## Tests

Unit tests use [Google Test](https://github.com/google/googletest) and cover the core logic and services layers.

### Test files

| File | Covers |
|------|--------|
| `TestTrade.cpp` | Trade profit/loss calculations at various price points |
| `TestTradeBuilder.cpp` | Multi-leg trade construction from option chains |
| `TestScorer.cpp` | Scoring and probability calculations |
| `TestNormalizer.cpp` | Score normalization to 0–100 scale |
| `TestWeigher.cpp` | Weighted metric combination |
| `TestMathUtils.cpp` | Normal distribution CDF and probability utilities |
| `TestSettingsManager.cpp` | Settings load/save in JSON format |
| `TestMarketDataImporter.cpp` | Parsing JSON API responses into option chains |

### Running tests

Build the project first (tests are enabled by default via the `BUILD_TESTS` CMake option), then run the test executable.

#### Windows (Command Prompt / PowerShell)

```powershell
# Using CTest
cd out\build\x64-debug
ctest --output-on-failure

# Or run the executable directly
tests\OptionometerTests.exe
```

#### WSL

> **Note:** CTest does not work under WSL because the MSVC build generates Windows-style paths in its CTest files. Instead, run the test executable directly — WSL2 can execute Windows binaries via interop.

```bash
./out/build/x64-debug/tests/OptionometerTests.exe
```

To disable tests during build, configure with `-DBUILD_TESTS=OFF`.

## License

MIT — see [LICENSE](LICENSE) for details.
