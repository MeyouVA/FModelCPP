# FModel — the Qt Widgets application (C++ port of the WPF FModel app). Built only when Qt6 is available
# (guarded in the root CMakeLists so the CUE4Parse library + tests still build without Qt).
find_package(Qt6 REQUIRED COMPONENTS Widgets Test)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# The settings tree, shared by the app and by test_settings.
set(FMODEL_SETTINGS_SOURCES
    Enums.h
    Extensions/JsonExtensions.h
    Extensions/Themes/JsonHighlightThemes.h
    Views/Snooper/Camera.h
    Framework/ViewModel.h
    Framework/ViewModel.cpp
    Framework/InputKeys.h
    Framework/GridLength.h
    Framework/Hotkey.h
    Framework/Hotkey.cpp
    ViewModels/GameSelectorViewModel.h
    ViewModels/GameSelectorViewModel.cpp
    ViewModels/ApiEndpoints/Models/AesResponse.h
    ViewModels/ApiEndpoints/Models/AesResponse.cpp
    ViewModels/ApiEndpoints/Models/EpicResponse.h
    ViewModels/ApiEndpoints/Models/EpicResponse.cpp
    Settings/CustomDirectory.h
    Settings/CustomDirectory.cpp
    Settings/EndpointSettings.h
    Settings/EndpointSettings.cpp
    Settings/VersioningSettings.h
    Settings/VersioningSettings.cpp
    Settings/DirectorySettings.h
    Settings/DirectorySettings.cpp
    Settings/UserSettings.h
    Settings/UserSettings.cpp
)

# The root view-model and what it pulls in. Shared by the app and by test_application_viewmodel; it needs the
# settings tree too, so this list is always used alongside FMODEL_SETTINGS_SOURCES.
set(FMODEL_VIEWMODEL_SOURCES
    Constants.h
    Constants.cpp
    Extensions/AssetCategoryExtensions.h
    Framework/FStatus.h
    Framework/FStatus.cpp
    ViewModels/LoadingModesViewModel.h
    ViewModels/LoadingModesViewModel.cpp
    ViewModels/ApplicationViewModel.h
    ViewModels/ApplicationViewModel.cpp
)

add_executable(FModel
    main.cpp
    MainWindow.h
    MainWindow.cpp
    Theme.h
    Theme.cpp
    # Framework — the MVVM base layer (ported from FModel/Framework).
    Framework/Command.h
    Framework/ViewModelCommand.h
    ${FMODEL_VIEWMODEL_SOURCES}
    ${FMODEL_SETTINGS_SOURCES}
    resources.qrc
)

# The app's own directory is an include root so ported files can use paths relative to the FModel project root,
# the way the C# source's namespaces are laid out.
target_include_directories(FModel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# CUE4Parse is the parsing core the app is built on; link it now so the combined build is validated even though
# the shell slice does not call into it yet. CUE4Parse-Conversion currently supplies only the export-format
# enums that UserSettings persists.
target_link_libraries(FModel PRIVATE Qt6::Widgets CUE4Parse CUE4Parse-Conversion)

# Copy the required Qt runtime DLLs next to the executable so it can be launched from the build tree (Windows).
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${Qt6_DIR}/../../../bin")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET FModel POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --no-compiler-runtime "$<TARGET_FILE:FModel>"
            COMMENT "Deploying Qt runtime with windeployqt")
    endif()
endif()

# Behavioural tests for the ported Framework layer (QtTest, registered with ctest alongside the CUE4Parse tests).
add_executable(test_framework
    tests/test_framework.cpp
    Enums.h
    Framework/ViewModel.h
    Framework/ViewModel.cpp
    Framework/Command.h
    Framework/ViewModelCommand.h
    Framework/FStatus.h
    Framework/FStatus.cpp
)
target_include_directories(test_framework PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_framework PRIVATE Qt6::Widgets Qt6::Test)
add_test(NAME test_framework COMMAND test_framework)
# test_framework pulls in Qt6Test (which FModel itself does not), so deploy its runtime DLLs too.
if(WIN32 AND WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET test_framework POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --no-compiler-runtime "$<TARGET_FILE:test_framework>"
        COMMENT "Deploying Qt runtime for test_framework")
endif()

# Behavioural tests for the ported Settings layer: defaults, the persisted JSON shape, and round-tripping.
add_executable(test_settings
    tests/test_settings.cpp
    ${FMODEL_SETTINGS_SOURCES}
)
target_include_directories(test_settings PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_settings PRIVATE Qt6::Widgets Qt6::Test CUE4Parse CUE4Parse-Conversion)
add_test(NAME test_settings COMMAND test_settings)
if(WIN32 AND WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET test_settings POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --no-compiler-runtime "$<TARGET_FILE:test_settings>"
        COMMENT "Deploying Qt runtime for test_settings")
endif()

# Behavioural tests for the root view-model layer: the guarded view-state properties, the category helpers,
# the loading-mode list and the constants/EGameName the window title is built from.
add_executable(test_application_viewmodel
    tests/test_application_viewmodel.cpp
    ${FMODEL_VIEWMODEL_SOURCES}
    ${FMODEL_SETTINGS_SOURCES}
)
target_include_directories(test_application_viewmodel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_application_viewmodel PRIVATE Qt6::Widgets Qt6::Test CUE4Parse CUE4Parse-Conversion)
add_test(NAME test_application_viewmodel COMMAND test_application_viewmodel)
if(WIN32 AND WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET test_application_viewmodel POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --no-compiler-runtime "$<TARGET_FILE:test_application_viewmodel>"
        COMMENT "Deploying Qt runtime for test_application_viewmodel")
endif()
