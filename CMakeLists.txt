cmake_minimum_required(VERSION 3.26)
project(GameVolumeHotkey LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)
# 將原來的第 10 行替換為：
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
# Qt6 — Windows 不需要 DBus
find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/trayicon.cpp
    src/settings_dialog.cpp
    src/hotkey_manager.cpp
    src/audio_manager.cpp
    src/config_manager.cpp
    src/logger.cpp
    src/single_instance.cpp
    src/hotkey_recorder.cpp
    include/mainwindow.h
    include/trayicon.h
    include/settings_dialog.h
    include/hotkey_manager.h
    include/audio_manager.h
    include/config_manager.h
    include/logger.h
    include/single_instance.h
    include/hotkey_recorder.h
    ui/settings_dialog.ui
    resources/resources.qrc
)

add_executable(GameVolumeHotkey WIN32 ${PROJECT_SOURCES})

# Windows 系統函式庫
# endpointvolume / avrt 不是獨立 lib，功能包含在 mmdevapi.lib 內
target_link_libraries(GameVolumeHotkey PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    user32
    ole32
    oleaut32
    mmdevapi
    shlwapi
    advapi32
    shell32
)

if(MSVC)
    target_compile_options(GameVolumeHotkey PRIVATE
        /W4
        /utf-8
        /wd4100   # unreferenced formal parameter
        /wd4251   # STL dll-interface
    )
    target_compile_definitions(GameVolumeHotkey PRIVATE
        UNICODE _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _WIN32_WINNT=0x0A00   # Windows 10+
    )
endif()

# DPI Aware (VS generator 會注入 manifest 項目)
set_target_properties(GameVolumeHotkey PROPERTIES VS_DPI_AWARE "PerMonitor")

# 編譯後自動執行 windeployqt，讓輸出目錄包含所有 Qt 執行期檔案
if(WIN32)
    get_target_property(_qmake Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin "${_qmake}" DIRECTORY)
    find_program(WINDEPLOYQT windeployqt HINTS "${_qt_bin}")
    if(WINDEPLOYQT)
        add_custom_command(TARGET GameVolumeHotkey POST_BUILD
            COMMAND "${WINDEPLOYQT}"
                --no-translations
                --no-system-d3d-compiler
                --no-opengl-sw
                --no-quick-import
                "$<TARGET_FILE:GameVolumeHotkey>"
            COMMENT "windeployqt..."
            VERBATIM
        )
    endif()
endif()
