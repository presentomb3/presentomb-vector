cmake_minimum_required(VERSION 3.22)

project(PresentombVectorVST VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(PRESENTOMB_FORMATS VST3 AU Standalone)
else()
    set(PRESENTOMB_FORMATS VST3 Standalone)
endif()

if(DEFINED JUCE_DIR)
    add_subdirectory("${JUCE_DIR}" JUCE)
elseif(EXISTS "D:/ableton plugin/build/_deps/juce-src/CMakeLists.txt")
    add_subdirectory("D:/ableton plugin/build/_deps/juce-src" JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.8
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(PresentombDSO2
    COMPANY_NAME "presentomb"
    BUNDLE_ID "com.presentomb.vectorvst"
    PLUGIN_MANUFACTURER_CODE PrTb
    PLUGIN_CODE VcT1
    FORMATS ${PRESENTOMB_FORMATS}
    PRODUCT_NAME "presentomb vector"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(PresentombDSO2
    PRIVATE
        src/PluginProcessor.cpp
        src/PluginEditor.cpp
        src/ScopeBuffer.cpp
        src/State/PresetManager.cpp
)

if(WIN32)
    target_sources(PresentombDSO2 PRIVATE src/DesktopLoopbackCapture.cpp)
    target_link_libraries(PresentombDSO2 PRIVATE ole32 uuid)
endif()

target_include_directories(PresentombDSO2 PRIVATE src)

target_compile_definitions(PresentombDSO2
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(PresentombDSO2
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
