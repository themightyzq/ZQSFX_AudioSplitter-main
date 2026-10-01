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
    ${ZQ_SOURCE_DIR}/Utils/SincResampler.cpp
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


# -----------------------------------------------------------------------------
# SplitSafetyTest -- data safety (temp-then-replace, verified writes, cancel) and the claims the
# README makes (32 channels, sample-rate override, 32-bit float, stereo-to-mono), proven through
# the real AudioFileProcessor.
# -----------------------------------------------------------------------------
juce_add_console_app(SplitSafetyTest
    PRODUCT_NAME "SplitSafetyTest"
)

target_sources(SplitSafetyTest PRIVATE
    SplitSafetyTest.cpp
    ${ZQ_SOURCE_DIR}/Utils/AudioFileProcessor.cpp
    ${ZQ_SOURCE_DIR}/Utils/SincResampler.cpp
    ${ZQ_SOURCE_DIR}/Utils/BroadcastChunkPreserver.cpp
)

target_include_directories(SplitSafetyTest PRIVATE
    ${ZQ_SOURCE_DIR}/Utils
)

target_compile_definitions(SplitSafetyTest PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1
)

juce_generate_juce_header(SplitSafetyTest)

target_link_libraries(SplitSafetyTest PRIVATE
    juce::juce_core
    juce::juce_events
    juce::juce_data_structures
    juce::juce_audio_basics
    juce::juce_audio_formats
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_test(NAME SplitSafety COMMAND SplitSafetyTest)

# -----------------------------------------------------------------------------
# BatchCancelTest -- a cancelled batch still completes (once, wasCancelled) and leaves no partial
# files. Uses BatchJobManager + BatchProcessingJob + ThreadPool directly.
# -----------------------------------------------------------------------------
juce_add_console_app(BatchCancelTest
    PRODUCT_NAME "BatchCancelTest"
)

target_sources(BatchCancelTest PRIVATE
    BatchCancelTest.cpp
    ${ZQ_SOURCE_DIR}/Utils/AudioFileProcessor.cpp
    ${ZQ_SOURCE_DIR}/Utils/SincResampler.cpp
    ${ZQ_SOURCE_DIR}/Utils/BroadcastChunkPreserver.cpp
    ${ZQ_SOURCE_DIR}/Utils/BatchJobManager.cpp
    ${ZQ_SOURCE_DIR}/Utils/BatchProcessingJob.cpp
)

target_include_directories(BatchCancelTest PRIVATE
    ${ZQ_SOURCE_DIR}/Utils
)

target_compile_definitions(BatchCancelTest PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1
    JUCE_MODAL_LOOPS_PERMITTED=1  # test pumps the message loop to receive completion callbacks
)

juce_generate_juce_header(BatchCancelTest)

target_link_libraries(BatchCancelTest PRIVATE
    juce::juce_core
    juce::juce_events
    juce::juce_data_structures
    juce::juce_audio_basics
    juce::juce_audio_formats
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_test(NAME BatchCancel COMMAND BatchCancelTest)

# -----------------------------------------------------------------------------
# BatchLifecycleTest -- BatchRun's lifetime rules (destroy mid-run, destroy from the completion
# handler, cancel). Built with AddressSanitizer where available so a use-after-free aborts it.
# -----------------------------------------------------------------------------
juce_add_console_app(BatchLifecycleTest
    PRODUCT_NAME "BatchLifecycleTest"
)

target_sources(BatchLifecycleTest PRIVATE
    BatchLifecycleTest.cpp
    ${ZQ_SOURCE_DIR}/Utils/AudioFileProcessor.cpp
    ${ZQ_SOURCE_DIR}/Utils/SincResampler.cpp
    ${ZQ_SOURCE_DIR}/Utils/BroadcastChunkPreserver.cpp
    ${ZQ_SOURCE_DIR}/Utils/BatchJobManager.cpp
    ${ZQ_SOURCE_DIR}/Utils/BatchProcessingJob.cpp
    ${ZQ_SOURCE_DIR}/Utils/BatchRun.cpp
)

target_include_directories(BatchLifecycleTest PRIVATE
    ${ZQ_SOURCE_DIR}/Utils
)

target_compile_definitions(BatchLifecycleTest PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1
    JUCE_MODAL_LOOPS_PERMITTED=1
)

juce_generate_juce_header(BatchLifecycleTest)

target_link_libraries(BatchLifecycleTest PRIVATE
    juce::juce_core
    juce::juce_events
    juce::juce_data_structures
    juce::juce_audio_basics
    juce::juce_audio_formats
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

option(ZQ_NO_TEST_ASAN "Build BatchLifecycleTest without AddressSanitizer" OFF)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU" AND NOT ZQ_NO_TEST_ASAN)
    target_compile_options(BatchLifecycleTest PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(BatchLifecycleTest PRIVATE -fsanitize=address)
endif()

add_test(NAME BatchLifecycle COMMAND BatchLifecycleTest)

# The test is about use-after-free. Leak reports from JUCE's process-lifetime statics and the
# Linux display stack are noise here, and would fail it for reasons unrelated to what it checks.
set_tests_properties(BatchLifecycle PROPERTIES ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0")
