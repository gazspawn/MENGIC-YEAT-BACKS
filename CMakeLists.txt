cmake_minimum_required(VERSION 3.22)
project(MENGIC_YEAT_BACKS VERSION 2.1.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(MENGIC_YEAT_BACKS
    COMPANY_NAME "MENGIC"
    BUNDLE_ID "com.mengic.alien"
    PLUGIN_MANUFACTURER_CODE MNGC
    PLUGIN_CODE MYAL
    FORMATS VST3
    PRODUCT_NAME "MENGIC ALIEN"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(MENGIC_YEAT_BACKS)

target_sources(MENGIC_YEAT_BACKS PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h)

target_compile_features(MENGIC_YEAT_BACKS PRIVATE cxx_std_17)
target_compile_definitions(MENGIC_YEAT_BACKS PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(MENGIC_YEAT_BACKS PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)

enable_testing()
add_executable(AlienDSPTests Tests/DSPTests.cpp)
target_compile_features(AlienDSPTests PRIVATE cxx_std_17)
add_test(NAME AlienDSPTests COMMAND AlienDSPTests)

juce_add_console_app(AlienValidation PRODUCT_NAME "AlienValidation")
target_sources(AlienValidation PRIVATE Tests/PluginValidation.cpp)
target_include_directories(AlienValidation PRIVATE Source
    "${CMAKE_CURRENT_BINARY_DIR}/MENGIC_YEAT_BACKS_artefacts/JuceLibraryCode")
target_compile_features(AlienValidation PRIVATE cxx_std_17)
target_compile_definitions(AlienValidation PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(AlienValidation PRIVATE MENGIC_YEAT_BACKS juce::juce_audio_utils juce::juce_dsp)
add_test(NAME AlienValidation COMMAND AlienValidation)
