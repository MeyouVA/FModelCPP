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
    Extensions/EnumExtensions.h
    ViewModels/LoadingModesViewModel.h
    ViewModels/LoadingModesViewModel.cpp
    ViewModels/SettingsViewModel.h
    ViewModels/SettingsViewModel.cpp
    ViewModels/ApplicationViewModel.h
    ViewModels/ApplicationViewModel.cpp
    # The explorer's folder tree. The command layer below reaches into it (MenuCommand's TreeItem arm and
    # its collapse walk, RightClickMenuCommand's folder half), so it has to come first.
    Framework/RangeObservableCollection.h
    Framework/CollectionView.h
    ViewModels/GameFileViewModel.h
    ViewModels/GameFileViewModel.cpp
    ViewModels/AssetsListViewModel.h
    ViewModels/AssetsListViewModel.cpp
    ViewModels/AssetsFolderViewModel.h
    ViewModels/AssetsFolderViewModel.cpp
    # The game-loading pipeline: the job funnel, the archives list, the provider host and the AES manager.
    Framework/AsyncQueue.h
    ViewModels/ThreadWorkerViewModel.h
    ViewModels/ThreadWorkerViewModel.cpp
    ViewModels/GameDirectoryViewModel.h
    ViewModels/GameDirectoryViewModel.cpp
    ViewModels/CUE4ParseViewModel.h
    ViewModels/CUE4ParseViewModel.cpp
    Helper.h
    Helper.cpp
    Framework/FullyObservableCollection.h
    ViewModels/AesManagerViewModel.h
    ViewModels/AesManagerViewModel.cpp
    # The command layer the root view-model exposes (ported from FModel/ViewModels/Commands).
    Framework/Command.h
    Framework/ViewModelCommand.h
    ViewModels/Commands/CopyCommand.h
    ViewModels/Commands/CopyCommand.cpp
    ViewModels/Commands/MenuCommand.h
    ViewModels/Commands/MenuCommand.cpp
    ViewModels/Commands/RightClickMenuCommand.h
    ViewModels/Commands/RightClickMenuCommand.cpp
    ViewModels/Commands/LoadCommand.h
    ViewModels/Commands/LoadCommand.cpp
)

# The Settings window and the two editors it opens (ported from FModel/Views). Needs both lists above.
set(FMODEL_SETTINGS_VIEW_SOURCES
    Views/Resources/Controls/DictionaryEditor.h
    Views/Resources/Controls/DictionaryEditor.cpp
    Views/Resources/Controls/EndpointEditor.h
    Views/Resources/Controls/EndpointEditor.cpp
    Views/SettingsView.h
    Views/SettingsView.cpp
)

# The two windows the game-loading path needs: pick a directory, then supply its AES keys.
set(FMODEL_LOADING_VIEW_SOURCES
    Views/DirectorySelector.h
    Views/DirectorySelector.cpp
    Views/AesManager.h
    Views/AesManager.cpp
)

add_executable(FModel
    main.cpp
    MainWindow.h
    MainWindow.cpp
    Theme.h
    Theme.cpp
    # The Framework MVVM base layer comes in with FMODEL_VIEWMODEL_SOURCES, which the command layer needs.
    ${FMODEL_VIEWMODEL_SOURCES}
    ${FMODEL_SETTINGS_SOURCES}
    ${FMODEL_SETTINGS_VIEW_SOURCES}
    ${FMODEL_LOADING_VIEW_SOURCES}
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

# Behavioural tests for the Settings window slice: the view-model's snapshot/save cycle, the JSON editors and
# the window's tree, browse and key-parsing behaviour.
add_executable(test_settings_view
    tests/test_settings_view.cpp
    ${FMODEL_SETTINGS_VIEW_SOURCES}
    ${FMODEL_VIEWMODEL_SOURCES}
    ${FMODEL_SETTINGS_SOURCES}
)
target_include_directories(test_settings_view PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_settings_view PRIVATE Qt6::Widgets Qt6::Test CUE4Parse CUE4Parse-Conversion)
add_test(NAME test_settings_view COMMAND test_settings_view)
if(WIN32 AND WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET test_settings_view POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --no-compiler-runtime "$<TARGET_FILE:test_settings_view>"
        COMMENT "Deploying Qt runtime for test_settings_view")
endif()

# Optional scratch harness that drives the whole loading pipeline headlessly against a real install.
# Off by default; configure with -DFMODEL_LOADGAME_HARNESS=<path to a .cpp> to build it.
if(FMODEL_LOADGAME_HARNESS)
    add_executable(loadgame ${FMODEL_LOADGAME_HARNESS}
        ${FMODEL_VIEWMODEL_SOURCES} ${FMODEL_SETTINGS_SOURCES})
    target_include_directories(loadgame PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(loadgame PRIVATE Qt6::Widgets CUE4Parse CUE4Parse-Conversion)
    if(WIN32 AND WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET loadgame POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --no-compiler-runtime "$<TARGET_FILE:loadgame>")
    endif()
endif()

# Behavioural tests for the explorer's folder tree: the bulk tree build, the search/category filters, the
# collection-view batching and GameFileViewModel's extension resolver.
add_executable(test_assets_folder
    tests/test_assets_folder.cpp
    ${FMODEL_VIEWMODEL_SOURCES}
    ${FMODEL_SETTINGS_SOURCES}
)
target_include_directories(test_assets_folder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_assets_folder PRIVATE Qt6::Widgets Qt6::Test CUE4Parse CUE4Parse-Conversion)
add_test(NAME test_assets_folder COMMAND test_assets_folder)
if(WIN32 AND WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET test_assets_folder POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --no-compiler-runtime "$<TARGET_FILE:test_assets_folder>"
        COMMENT "Deploying Qt runtime for test_assets_folder")
endif()

# Behavioural tests for the game-loading pipeline: the job funnel, the archives list, the AES key rows,
# LoadCommand's filters and the directory selector's list.
add_executable(test_loading
    tests/test_loading.cpp
    ${FMODEL_VIEWMODEL_SOURCES}
    ${FMODEL_SETTINGS_SOURCES}
)
target_include_directories(test_loading PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_loading PRIVATE Qt6::Widgets Qt6::Test CUE4Parse CUE4Parse-Conversion)
add_test(NAME test_loading COMMAND test_loading)
if(WIN32 AND WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET test_loading POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --no-compiler-runtime "$<TARGET_FILE:test_loading>"
        COMMENT "Deploying Qt runtime for test_loading")
endif()

# Behavioural tests for the command layer: the copy triggers, the menu dispatch (including which arms are
# still deferred), and RightClickMenuCommand's trigger/bulk tables, grouping and export paths.
add_executable(test_commands
    tests/test_commands.cpp
    ${FMODEL_VIEWMODEL_SOURCES}
    ${FMODEL_SETTINGS_SOURCES}
)
target_include_directories(test_commands PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_commands PRIVATE Qt6::Widgets Qt6::Test CUE4Parse CUE4Parse-Conversion)
add_test(NAME test_commands COMMAND test_commands)
if(WIN32 AND WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET test_commands POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --no-compiler-runtime "$<TARGET_FILE:test_commands>"
        COMMENT "Deploying Qt runtime for test_commands")
endif()
