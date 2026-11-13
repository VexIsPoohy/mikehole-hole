cmake_minimum_required(VERSION 3.22)

project(MikeHolesHole VERSION 1.0.0 LANGUAGES C CXX)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(MikeHolesHole
    COMPANY_NAME "VexIsPoohy"
    BUNDLE_ID "com.vexispoohy.mikeholeshole"
    PLUGIN_MANUFACTURER_CODE VIsP
    PLUGIN_CODE MkHH
    FORMATS VST3
    PRODUCT_NAME "Mike Holes Hole"
    DESCRIPTION "Granular space generator"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(MikeHolesHole
    PRIVATE
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/Presets.h
)

target_compile_features(MikeHolesHole PRIVATE cxx_std_17)

target_compile_definitions(MikeHolesHole
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(MikeHolesHole
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
