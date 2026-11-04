cmake_minimum_required(VERSION 3.22)

# These two must come before project(), or the plugins are built for the macOS of the build machine.
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
if(CMAKE_HOST_APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures" FORCE)
endif()

project(SonicGrain VERSION 0.2.6 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(SONICGRAIN_BUILD_AAX "Also build AAX (Pro Tools; needs PACE signing to load in release Pro Tools)" OFF)
option(SONICGRAIN_BUILD_TESTS "Build the headless test runner" OFF)
option(SONICGRAIN_COPY_AFTER_BUILD "Install the plugins into the user plug-in folders after building" ON)
set(SONICGRAIN_AAX_COPY_DIR "" CACHE PATH "Where the AAX is copied after building (empty = JUCE default, the system Avid folder, needs write access)")

# ---- JUCE: -DJUCE_DIR=<path>, or ./JUCE, or downloaded automatically (needs git)
set(SONICGRAIN_JUCE_TAG "8.0.10" CACHE STRING "JUCE version fetched when no local copy is given")
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt)
    set(JUCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/JUCE)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG ${SONICGRAIN_JUCE_TAG}
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE)
    FetchContent_MakeAvailable(JUCE)
    set(JUCE_DIR ${juce_SOURCE_DIR})
endif()

set(SONICGRAIN_FORMATS VST3)
if(APPLE)
    list(APPEND SONICGRAIN_FORMATS AU)
endif()
if(SONICGRAIN_BUILD_AAX)
    if(EXISTS ${JUCE_DIR}/modules/juce_audio_plugin_client/AAX/SDK)
        juce_set_aax_sdk_path(${JUCE_DIR}/modules/juce_audio_plugin_client/AAX/SDK)
    endif()
    list(APPEND SONICGRAIN_FORMATS AAX)
endif()

set(SONICGRAIN_EXTRA_PLUGIN_ARGS)
if(SONICGRAIN_AAX_COPY_DIR)
    list(APPEND SONICGRAIN_EXTRA_PLUGIN_ARGS AAX_COPY_DIR "${SONICGRAIN_AAX_COPY_DIR}")
endif()

juce_add_plugin(SonicGrain
    ${SONICGRAIN_EXTRA_PLUGIN_ARGS}
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "Gianluca Sofia"
    BUNDLE_ID "com.gianlucasofia.sonicgrain"
    PLUGIN_MANUFACTURER_CODE Gsof
    PLUGIN_CODE Sgrn
    PRODUCT_NAME "SONIC GRAIN"
    FORMATS ${SONICGRAIN_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD ${SONICGRAIN_COPY_AFTER_BUILD}
    VST3_CATEGORIES Fx Delay Modulation
    AU_MAIN_TYPE kAudioUnitType_Effect
    AAX_CATEGORY AAX_ePlugInCategory_Delay)

# Embedded UI fonts (Rubik, Silkscreen; SIL Open Font License, see Resources/fonts) and the logo mark
juce_add_binary_data(SonicGrainFonts HEADER_NAME SonicFonts.h NAMESPACE SonicFonts SOURCES
    Resources/logo/sonicgrain-mark.svg
    Resources/fonts/Rubik-Medium.ttf
    Resources/fonts/Rubik-Bold.ttf
    Resources/fonts/Rubik-BlackItalic.ttf
    Resources/fonts/Silkscreen-Regular.ttf)
set_target_properties(SonicGrainFonts PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_sources(SonicGrain PRIVATE Source/SonicProcessor.cpp)
target_include_directories(SonicGrain PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)
target_compile_definitions(SonicGrain PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
    SONICGRAIN_VERSION="${PROJECT_VERSION}")
target_link_libraries(SonicGrain
    PRIVATE SonicGrainFonts juce::juce_audio_utils juce::juce_audio_processors juce::juce_gui_basics juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(SONICGRAIN_BUILD_TESTS)
    add_subdirectory(Tests)
endif()
