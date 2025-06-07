/**
 * Taffy Audio Tools
 * Helper utilities for creating audio assets programmatically
 */

#pragma once

#include <string>
#include <cstdint>
#include <vector>

namespace tremor::taffy::tools {

    /**
     * Create a waveform-based audio asset
     * @param output_path Path to save the .taf file
     * @param frequency Oscillator frequency in Hz (ignored for noise)
     * @param duration Duration in seconds
     * @param waveform_type Type of waveform (0=Sine, 1=Square, 2=Saw, 3=Triangle, 4=Noise)
     * @return true if successful
     */
    bool createWaveformAudioAsset(const std::string& output_path, 
                                  float frequency = 440.0f, 
                                  float duration = 1.0f, 
                                  uint32_t waveform_type = 0);
    
    /**
     * Create a sine wave audio asset (convenience function)
     * @param output_path Path to save the .taf file
     * @param frequency Frequency in Hz
     * @param duration Duration in seconds
     * @return true if successful
     */
    bool createSineWaveAudioAsset(const std::string& output_path, 
                                  float frequency = 440.0f, 
                                  float duration = 1.0f);

    /**
     * Create a mixer demo audio asset that combines multiple waveforms
     * @param output_path Path to save the .taf file
     * @param duration Duration in seconds
     * @return true if successful
     */
    bool createMixerDemoAsset(const std::string& output_path,
                              float duration = 2.0f);

    /**
     * Create an ADSR envelope demo audio asset
     * @param output_path Path to save the .taf file
     * @param duration Duration in seconds
     * @return true if successful
     */
    bool createADSRDemoAsset(const std::string& output_path,
                             float duration = 3.0f);

    /**
     * Create a filter demo audio asset
     * @param output_path Path to save the .taf file
     * @param filterType Filter type (0=Lowpass, 1=Highpass, 2=Bandpass)
     * @param duration Duration in seconds
     * @return true if successful
     */
    bool createFilterDemoAsset(const std::string& output_path,
                               uint32_t filterType = 0,
                               float duration = 3.0f);

    /**
     * Create a distortion demo audio asset
     * @param output_path Path to save the .taf file
     * @param distortionType Distortion type (0=HardClip, 1=SoftClip, 2=Foldback, 3=BitCrush, 4=Overdrive)
     * @param duration Duration in seconds
     * @return true if successful
     */
    bool createDistortionDemoAsset(const std::string& output_path,
                                   uint32_t distortionType = 0,
                                   float duration = 3.0f);

    /**
     * Create a sample-based audio asset
     * @param output_path Path to save the .taf file
     * @param sampleData Sample data (interleaved if stereo)
     * @param sampleRate Sample rate in Hz
     * @param channelCount Number of channels (1=mono, 2=stereo)
     * @param baseFrequency Base frequency for pitch shifting
     * @param loopStart Loop start point in samples (0 = no loop)
     * @param loopEnd Loop end point in samples (0 = no loop)
     * @return true if successful
     */
    bool createSampleAudioAsset(const std::string& output_path,
                               const std::vector<float>& sampleData,
                               uint32_t sampleRate = 48000,
                               uint32_t channelCount = 1,
                               float baseFrequency = 440.0f,
                               uint32_t loopStart = 0,
                               uint32_t loopEnd = 0);

    /**
     * Load WAV file data (simple implementation)
     * @param wavPath Path to WAV file
     * @param outData Output sample data
     * @param outSampleRate Output sample rate
     * @param outChannelCount Output channel count
     * @return true if successful
     */
    bool loadWAVFile(const std::string& wavPath,
                     std::vector<float>& outData,
                     uint32_t& outSampleRate,
                     uint32_t& outChannelCount);

} // namespace tremor::taffy::tools