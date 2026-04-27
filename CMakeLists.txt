cmake_minimum_required(VERSION 3.14)
project(tgcli VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Fetch FTXUI ──────────────────────────────────────────────────────────────
include(FetchContent)

FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
    GIT_TAG        v5.0.0
)
FetchContent_MakeAvailable(ftxui)

# ── Fetch nlohmann/json ──────────────────────────────────────────────────────
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

# ── Find TDLib (must be pre-built and installed) ─────────────────────────────
# Set Td_DIR to the TDLib install path if not in standard locations
# e.g. cmake .. -DTd_DIR=/path/to/td/build/install/lib/cmake/Td
find_package(Td REQUIRED)

# ── Find libcurl for HTTP requests (ExteraGram API) ─────────────────────────
find_package(CURL REQUIRED)

# ── Find libqrencode for QR code terminal display ───────────────────────────
find_package(PkgConfig REQUIRED)
pkg_check_modules(QRENCODE REQUIRED libqrencode)

# ── Source files ─────────────────────────────────────────────────────────────
set(TGCLI_SOURCES
    src/main.cpp
    src/app/App.cpp
    src/app/Config.cpp
    src/app/ExteraGram.cpp
    src/telegram/Client.cpp
    src/telegram/Auth.cpp
    src/telegram/Messages.cpp
    src/telegram/Stars.cpp
    src/telegram/Gifts.cpp
    src/ui/ChatList.cpp
    src/ui/ChatView.cpp
    src/ui/InputBar.cpp
    src/ui/StatusBar.cpp
    src/ui/StarsPanel.cpp
    src/ui/GiftsPanel.cpp
    src/ui/InfoPanel.cpp
    src/ui/AuthScreen.cpp
)

add_executable(tgcli ${TGCLI_SOURCES})

target_include_directories(tgcli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${QRENCODE_INCLUDE_DIRS}
)

target_link_libraries(tgcli PRIVATE
    ftxui::screen
    ftxui::dom
    ftxui::component
    nlohmann_json::nlohmann_json
    Td::TdStatic
    CURL::libcurl
    ${QRENCODE_LIBRARIES}
    pthread
)

# ── Install as both 'tgcli' and 'telegram' ──────────────────────────────────
install(TARGETS tgcli DESTINATION bin)

# Create a 'telegram' symlink so the user can type 'telegram' to launch
install(CODE "
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
        \"${CMAKE_INSTALL_PREFIX}/bin/tgcli\"
        \"${CMAKE_INSTALL_PREFIX}/bin/telegram\")
")
