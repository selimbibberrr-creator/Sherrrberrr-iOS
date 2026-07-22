cmake_minimum_required(VERSION 3.22)

project(GabberShermanKick VERSION 1.0.0 LANGUAGES C CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(GSK_ENABLE_LTO "Enable link-time optimisation for distributable builds" OFF)
option(GSK_BUILD_DESKTOP_QA "Build desktop-only VST3/editor smoke targets" ON)

set(GSK_BUILD_VERSION "1.0.0" CACHE STRING "Apple CFBundleVersion/build version")
set(GSK_IOS_BUNDLE_ID "com.selimbibberrr.sherrrberrr" CACHE STRING "iOS standalone bundle identifier")
set(GSK_DESKTOP_BUNDLE_ID "com.codexhardcorelab.kickflavoredsherberrr.v4" CACHE STRING "Desktop plugin bundle identifier")

include(FetchContent)

# The app icon is generated deterministically from source so the Git repository
# stays text-first and Codemagic/local builds get the exact same 1024px RGB asset.
set(GSK_APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/Assets/app-icon-1024.png")
if(NOT EXISTS "${GSK_APP_ICON}")
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_app_icon.py"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE GSK_ICON_RESULT
    )
    if(NOT GSK_ICON_RESULT EQUAL 0 OR NOT EXISTS "${GSK_APP_ICON}")
        message(FATAL_ERROR "Failed to generate ${GSK_APP_ICON}")
    endif()
endif()

set(GABBER_SHERMAN_JUCE_GIT_TAG "8.0.10" CACHE STRING "JUCE git tag to use when JUCE_DIR is not provided")

if(DEFINED JUCE_DIR AND EXISTS "${JUCE_DIR}/CMakeLists.txt")
    add_subdirectory("${JUCE_DIR}" JUCE)
else()
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG ${GABBER_SHERMAN_JUCE_GIT_TAG}
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "Minimum iOS version" FORCE)
    endif()
    set(CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2")
    set(GSK_PLUGIN_FORMATS AUv3 Standalone)
    set(GSK_MANUFACTURER_CODE Cdxh)
    set(GSK_PRODUCT_NAME "Kick flavored SherrrBerrr")
    set(GSK_BUNDLE_ID "${GSK_IOS_BUNDLE_ID}")
    set(GSK_IS_IOS TRUE)
else()
    set(GSK_PLUGIN_FORMATS VST3 Standalone)
    set(GSK_MANUFACTURER_CODE CdxH)
    set(GSK_PRODUCT_NAME "Kick flavored SherrrBerrr")
    set(GSK_BUNDLE_ID "${GSK_DESKTOP_BUNDLE_ID}")
    set(GSK_IS_IOS FALSE)
endif()

juce_add_plugin(GabberShermanKick
    VERSION ${PROJECT_VERSION}
    BUILD_VERSION ${GSK_BUILD_VERSION}
    COMPANY_NAME "SherrrBerrr"
    BUNDLE_ID ${GSK_BUNDLE_ID}
    PLUGIN_NAME "SherrrBerrr"
    DESCRIPTION "Touch-first hardcore kick synthesizer and sound-design instrument"
    ICON_BIG "${GSK_APP_ICON}"
    ICON_SMALL "${GSK_APP_ICON}"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE ${GSK_MANUFACTURER_CODE}
    PLUGIN_CODE Gsk4
    FORMATS ${GSK_PLUGIN_FORMATS}
    VST3_AUTO_MANIFEST TRUE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    AU_SANDBOX_SAFE TRUE
    BACKGROUND_AUDIO_ENABLED TRUE
    FILE_SHARING_ENABLED TRUE
    DOCUMENT_BROWSER_ENABLED TRUE
    TARGETED_DEVICE_FAMILY "1,2"
    IPHONE_SCREEN_ORIENTATIONS UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight
    IPAD_SCREEN_ORIENTATIONS UIInterfaceOrientationPortrait UIInterfaceOrientationPortraitUpsideDown UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight
    PRODUCT_NAME ${GSK_PRODUCT_NAME}
)

juce_generate_juce_header(GabberShermanKick)

set(GSK_CORE_SOURCES
    Source/Dsp/KickEngine.cpp
    Source/Dsp/KickEngine.h
    Source/Dsp/DualFilterModule.cpp
    Source/Dsp/DualFilterModule.h
    Source/Dsp/DualDistortionModule.cpp
    Source/Dsp/DualDistortionModule.h
    Source/Dsp/MacroMapper.cpp
    Source/Dsp/MacroMapper.h
    Source/Dsp/SourceEnvelope.cpp
    Source/Dsp/SourceEnvelope.h
    Source/Parameters/KickParameters.cpp
    Source/Parameters/KickParameters.h
)

add_library(SherrrBerrrCore STATIC ${GSK_CORE_SOURCES})
target_link_libraries(SherrrBerrrCore
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_processors
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
target_include_directories(SherrrBerrrCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Source")
set_target_properties(SherrrBerrrCore PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
if(APPLE)
    set_target_properties(SherrrBerrrCore PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "./")
endif()

target_sources(GabberShermanKick
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/Platform/PlatformConfig.h
)

target_compile_definitions(GabberShermanKick
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS=0
        GSK_IOS_BUILD=$<BOOL:${GSK_IS_IOS}>
)

target_link_libraries(GabberShermanKick
    PRIVATE
        SherrrBerrrCore
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

if(GSK_ENABLE_LTO)
    target_link_libraries(GabberShermanKick PRIVATE juce::juce_recommended_lto_flags)
endif()

add_executable(GabberShermanKickCoreTests Source/Tests/KickEngineTests.cpp)
target_link_libraries(GabberShermanKickCoreTests
    PRIVATE
        SherrrBerrrCore
        juce::juce_audio_basics
        juce::juce_audio_processors
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_executable(GabberShermanKickRender Source/Tools/RenderKicks.cpp)
target_link_libraries(GabberShermanKickRender
    PRIVATE
        SherrrBerrrCore
        juce::juce_audio_basics
        juce::juce_audio_formats
        juce::juce_audio_processors
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

if(NOT GSK_IS_IOS AND GSK_BUILD_DESKTOP_QA)
    add_executable(GabberShermanKickVst3Smoke Source/Tests/Vst3HostSmoke.cpp)
    add_executable(GabberShermanKickEditorSmoke Source/Tests/EditorSmoke.cpp)
    add_executable(GabberShermanKickLifecycleSmoke Source/Tests/ProcessorLifecycleTests.cpp)
    target_link_libraries(GabberShermanKickEditorSmoke PRIVATE GabberShermanKick juce::juce_audio_utils)
    target_include_directories(GabberShermanKickEditorSmoke PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/GabberShermanKick_artefacts/JuceLibraryCode")
    target_link_libraries(GabberShermanKickLifecycleSmoke PRIVATE GabberShermanKick juce::juce_audio_utils)
    target_include_directories(GabberShermanKickLifecycleSmoke PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/GabberShermanKick_artefacts/JuceLibraryCode")
    target_compile_definitions(GabberShermanKickVst3Smoke PRIVATE JUCE_PLUGINHOST_VST3=1 JUCE_PLUGINHOST_VST=0 JUCE_PLUGINHOST_AU=0 GSK_EXPECTED_PRODUCT_PARAMETERS=128)
    target_link_libraries(GabberShermanKickVst3Smoke PRIVATE juce::juce_audio_utils PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    add_dependencies(GabberShermanKickVst3Smoke GabberShermanKick_VST3)
endif()

if(BUILD_TESTING AND NOT GSK_IS_IOS)
    add_test(NAME GabberKickV5Core COMMAND GabberShermanKickCoreTests)
    set_tests_properties(GabberKickV5Core PROPERTIES LABELS "dsp;sources;adsr;state;macros;xy" TIMEOUT 180)
    add_test(NAME GabberKickV5RenderQA COMMAND GabberShermanKickRender qa)
    set_tests_properties(GabberKickV5RenderQA PROPERTIES LABELS "audio;qa" TIMEOUT 45)
    if(GSK_BUILD_DESKTOP_QA)
        add_test(NAME GabberKickV5Vst3Host COMMAND GabberShermanKickVst3Smoke "${CMAKE_CURRENT_BINARY_DIR}/GabberShermanKick_artefacts/$<CONFIG>/VST3/${GSK_PRODUCT_NAME}.vst3")
        set_tests_properties(GabberKickV5Vst3Host PROPERTIES LABELS "vst3;host;state;editor" TIMEOUT 45)
        add_test(NAME GabberKickV5Editor COMMAND GabberShermanKickEditorSmoke)
        set_tests_properties(GabberKickV5Editor PROPERTIES LABELS "ui;editor;layout" TIMEOUT 30)
        add_test(NAME GabberKickV8Lifecycle COMMAND GabberShermanKickLifecycleSmoke)
        set_tests_properties(GabberKickV8Lifecycle PROPERTIES LABELS "auv3;lifecycle;state;multi-instance" TIMEOUT 45)
    endif()
endif()
