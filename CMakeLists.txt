cmake_minimum_required(VERSION 3.22)

project(Kroppsmaskinen VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Lägsta macOS-version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary")

option(KM_BUILD_TOOLS "Bygg KMRender och KMSnapshot (renderingstest och bilder av panelen)" ON)

# --- Beroenden -------------------------------------------------------------
# Hämtas automatiskt. För att använda en lokal kopia (t.ex. den som KrangBox redan hämtat):
#   cmake -B build -DFETCHCONTENT_SOURCE_DIR_JUCE=/sökväg/till/JUCE
include(FetchContent)

FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE)

FetchContent_Declare(clap-juce-extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
    GIT_SUBMODULES_RECURSE TRUE)

FetchContent_MakeAvailable(JUCE clap-juce-extensions)

# --- Pluginet --------------------------------------------------------------
set(KM_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND KM_FORMATS AU)
endif()

juce_add_plugin(Kroppsmaskinen
    COMPANY_NAME                "Kroppsmaskinen"
    BUNDLE_ID                   "org.kroppsmaskinen.km87"
    PLUGIN_MANUFACTURER_CODE    Kpmk
    PLUGIN_CODE                 Km87
    PRODUCT_NAME                "Kroppsmaskinen"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Instrument Synth
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    FORMATS                     ${KM_FORMATS})

clap_juce_extensions_plugin(TARGET Kroppsmaskinen
    CLAP_ID "org.kroppsmaskinen.km87"
    CLAP_FEATURES instrument synthesizer drum-machine)

# clap-juce-extensions lägger inte CLAP-målet i Kroppsmaskinen_All; gör det här.
add_dependencies(Kroppsmaskinen_All Kroppsmaskinen_CLAP)

set(KM_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_sources(Kroppsmaskinen PRIVATE ${KM_SOURCES})
target_include_directories(Kroppsmaskinen PRIVATE Source)

set(KM_DEFINITIONS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)   # Tillåtet under AGPLv3 / JUCE:s fria licens

target_compile_definitions(Kroppsmaskinen PUBLIC ${KM_DEFINITIONS})

target_link_libraries(Kroppsmaskinen
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# --- Verktyg ---------------------------------------------------------------
# Båda verktygen bygger in samma processor och panel som pluginet.
if(KM_BUILD_TOOLS)
    foreach(tool KMRender KMSnapshot)
        juce_add_console_app(${tool} PRODUCT_NAME "${tool}")
        target_sources(${tool} PRIVATE tools/${tool}.cpp ${KM_SOURCES})
        target_include_directories(${tool} PRIVATE Source)
        target_compile_definitions(${tool} PRIVATE ${KM_DEFINITIONS})
        target_link_libraries(${tool}
            PRIVATE juce::juce_audio_utils juce::juce_audio_formats juce::juce_dsp
            PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    endforeach()
endif()
