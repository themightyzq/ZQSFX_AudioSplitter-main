# Test targets for ZQ SFX Audio Splitter.
# Built as separate console apps so they do not affect the GUI app build.
# Build explicitly:  cmake --build <dir> --target MetadataRoundTripTest SplitPipelineTest
# Run:               ctest --test-dir <dir> --output-on-failure

include(CTest)

set(ZQ_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../Source)

# -----------------------------------------------------------------------------
# MetadataRoundTripTest -- unit test of the JUCE round-trip + BroadcastChunkPreserver.
# -----------------------------------------------------------------------------
juce_add_console_app(MetadataRoundTripTest
    PRODUCT_NAME "MetadataRoundTripTest"
)

target_sources(MetadataRoundTripTest PRIVATE
    MetadataRoundTripTest.cpp
    ${ZQ_SOURCE_DIR}/Utils/BroadcastChunkPreserver.cpp
)

target_include_directories(MetadataRoundTripTest PRIVATE
    ${ZQ_SOURCE_DIR}/Utils
)

target_compile_definitions(MetadataRoundTripTest PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1
)

target_link_libraries(MetadataRoundTripTest PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_formats
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_test(NAME MetadataRoundTrip COMMAND MetadataRoundTripTest)

# -----------------------------------------------------------------------------
# SplitPipelineTest -- end-to-end: drives AudioFileProcessor through a real split.
# AudioFileProcessor.h includes <JuceHeader.h>, so this target must generate it.
# -----------------------------------------------------------------------------
juce_add_console_app(SplitPipelineTest
    PRODUCT_NAME "SplitPipelineTest"
)

target_sources(SplitPipelineTest PRIVATE
    SplitPipelineTest.cpp
    ${ZQ_SOURCE_DIR}/Utils/AudioFileProcessor.cpp
    ${ZQ_SOURCE_DIR}/Utils/BroadcastChunkPreserver.cpp
)

target_include_directories(SplitPipelineTest PRIVATE
    ${ZQ_SOURCE_DIR}/Utils
)

target_compile_definitions(SplitPipelineTest PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1
    JUCE_MODAL_LOOPS_PERMITTED=1  # test pumps the message loop to receive completion callbacks
)

juce_generate_juce_header(SplitPipelineTest)

target_link_libraries(SplitPipelineTest PRIVATE
    juce::juce_core
    juce::juce_events
    juce::juce_data_structures
    juce::juce_audio_basics
    juce::juce_audio_formats
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_test(NAME SplitPipeline COMMAND SplitPipelineTest)
