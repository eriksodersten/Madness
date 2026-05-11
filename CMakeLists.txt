cmake_minimum_required(VERSION 3.22)
project(PWMMadness VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(/Users/eriksode/JUCE JUCE)

juce_add_plugin(PWMMadness
    COMPANY_NAME "eriksodersten"
    PLUGIN_MANUFACTURER_CODE Esod
    PLUGIN_CODE Pwmd
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "PWM Madness"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
)

juce_generate_juce_header(PWMMadness)

target_sources(PWMMadness PRIVATE
    Source/PluginProcessor.h
    Source/PluginEditor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

juce_add_binary_data(PWMMadnessAssets
    SOURCES
        Source/Assets/vintage_synth_madness_and_beach_vibes.png
)

target_compile_definitions(PWMMadness PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(PWMMadness PRIVATE
    PWMMadnessAssets
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
