juce_add_console_app(SonicGrainTests PRODUCT_NAME "SonicGrainTests")
target_sources(SonicGrainTests PRIVATE TestMain.cpp ${PROJECT_SOURCE_DIR}/Source/SonicProcessor.cpp)
target_include_directories(SonicGrainTests PRIVATE ${PROJECT_SOURCE_DIR}/Source)
target_compile_definitions(SonicGrainTests PRIVATE
    IDM_NO_PLUGIN_ENTRY=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
    JucePlugin_Name="SonicGrainTests" JUCE_STANDALONE_APPLICATION=1)
target_link_libraries(SonicGrainTests PRIVATE
    juce::juce_audio_processors juce::juce_gui_basics juce::juce_audio_basics juce::juce_dsp
    juce::juce_recommended_config_flags)
