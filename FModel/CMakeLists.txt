# FModel — the Qt Widgets application (C++ port of the WPF FModel app). Built only when Qt6 is available
# (guarded in the root CMakeLists so the CUE4Parse library + tests still build without Qt).
find_package(Qt6 REQUIRED COMPONENTS Widgets Test)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(FModel
    main.cpp
    MainWindow.h
    MainWindow.cpp
    Theme.h
    Theme.cpp
    Enums.h
    # Framework — the MVVM base layer (ported from FModel/Framework).
    Framework/ViewModel.h
    Framework/ViewModel.cpp
    Framework/Command.h
    Framework/ViewModelCommand.h
    Framework/FStatus.h
    Framework/FStatus.cpp
    resources.qrc
)

# CUE4Parse is the parsing core the app is built on; link it now so the combined build is validated even though
# the shell slice does not call into it yet.
target_link_libraries(FModel PRIVATE Qt6::Widgets CUE4Parse)

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
