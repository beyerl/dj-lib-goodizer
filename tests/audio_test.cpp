#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <string>

#include "djcore/audio/AudioDecoder.h"
#include "djcore/audio/AudioEncoder.h"
#include "djcore/audio/Dither.h"
#include "djcore/audio/PcmBuffer.h"
#include "djcore/audio/Resampler.h"

using namespace djcore;

namespace {

constexpr double kPi = 3.14159265358979323846;

// A stereo sine: left at freqL, right at freqR, amplitude 0.5.
PcmBuffer makeSine(int sampleRate, std::size_t frames, double freqL, double freqR) {
  PcmBuffer buf(2, sampleRate, frames);
  for (std::size_t i = 0; i < frames; ++i) {
    const double t = static_cast<double>(i) / sampleRate;
    buf.channel(0)[i] = static_cast<float>(0.5 * std::sin(2.0 * kPi * freqL * t));
    buf.channel(1)[i] = static_cast<float>(0.5 * std::sin(2.0 * kPi * freqR * t));
  }
  return buf;
}

double rms(const float* x, std::size_t n) {
  double acc = 0.0;
  for (std::size_t i = 0; i < n; ++i) acc += static_cast<double>(x[i]) * x[i];
  return std::sqrt(acc / static_cast<double>(n));
}

std::string tempWav(const char* name) {
  return (std::filesystem::temp_directory_path() / name).string();
}

}  // namespace

TEST(Wav, Float32RoundTripIsExact) {
  const auto src = makeSine(44100, 4096, 1000.0, 1500.0);
  const std::string path = tempWav("djcore_f32.wav");
  encodeToFile(path, src, EncodeSpec{"wav", 44100, 32, /*isFloat=*/true});

  auto dec = openDecoder(path);
  EXPECT_EQ(dec->format().sampleRate, 44100);
  EXPECT_EQ(dec->format().channels, 2);
  const PcmBuffer out = dec->readAll();
  ASSERT_EQ(out.frames(), src.frames());
  for (int ch = 0; ch < 2; ++ch) {
    for (std::size_t i = 0; i < src.frames(); ++i) {
      EXPECT_NEAR(out.channel(ch)[i], src.channel(ch)[i], 1e-6f);
    }
  }
  std::filesystem::remove(path);
}

TEST(Wav, Pcm16RoundTripWithinQuantTolerance) {
  const auto src = makeSine(44100, 4096, 440.0, 660.0);
  const std::string path = tempWav("djcore_s16.wav");
  encodeToFile(path, src, EncodeSpec{"wav", 44100, 16, /*isFloat=*/false});

  auto dec = openDecoder(path);
  EXPECT_EQ(dec->format().bitDepth, 16);
  EXPECT_EQ(dec->format().codec, "pcm_s16le");
  const PcmBuffer out = dec->readAll();
  ASSERT_EQ(out.frames(), src.frames());
  // 16-bit step ~3e-5; TPDF dither adds up to ~2 LSB of error.
  for (std::size_t i = 0; i < src.frames(); ++i) {
    EXPECT_NEAR(out.channel(0)[i], src.channel(0)[i], 2e-4f);
  }
  std::filesystem::remove(path);
}

TEST(Wav, ReadBlockStreamsAllFrames) {
  const auto src = makeSine(48000, 3000, 500.0, 500.0);
  const std::string path = tempWav("djcore_block.wav");
  encodeToFile(path, src, EncodeSpec{"wav", 48000, 24, false});

  auto dec = openDecoder(path);
  PcmBuffer block;
  std::size_t total = 0;
  while (std::size_t n = dec->readBlock(block, 512)) {
    total += n;
  }
  EXPECT_EQ(total, src.frames());
  std::filesystem::remove(path);
}

TEST(Resampler, UpsamplePreservesLengthAndAmplitude) {
  const auto src = makeSine(44100, 44100, 1000.0, 1000.0);  // 1 s, 1 kHz
  const PcmBuffer up = resample(src, 48000);
  EXPECT_EQ(up.sampleRate(), 48000);
  EXPECT_NEAR(static_cast<double>(up.frames()), 48000.0, 2.0);
  // RMS of a 0.5-amplitude sine is 0.5/sqrt(2) ~= 0.3536; expect it preserved.
  const double rIn = rms(src.channel(0), src.frames());
  const double rOut = rms(up.channel(0), up.frames());
  EXPECT_NEAR(rOut, rIn, 0.02);
}

TEST(Resampler, DownsampleProducesExpectedLength) {
  const auto src = makeSine(48000, 48000, 1000.0, 1000.0);
  const PcmBuffer down = resample(src, 44100);
  EXPECT_EQ(down.sampleRate(), 44100);
  EXPECT_NEAR(static_cast<double>(down.frames()), 44100.0, 2.0);
  const double rOut = rms(down.channel(0), down.frames());
  EXPECT_NEAR(rOut, 0.3536, 0.02);
}

TEST(Dither, DeterministicAndInRange) {
  TpdfDither a(123), b(123);
  double sum = 0.0;
  const int n = 100000;
  for (int i = 0; i < n; ++i) {
    const std::int32_t ca = a.quantize(0.0f, 16);
    const std::int32_t cb = b.quantize(0.0f, 16);
    EXPECT_EQ(ca, cb);                 // same seed => same sequence
    EXPECT_GE(ca, -32768);
    EXPECT_LE(ca, 32767);
    sum += ca;
  }
  // Dithering silence should be zero-mean (±1 LSB triangular noise).
  EXPECT_NEAR(sum / n, 0.0, 0.05);
}
