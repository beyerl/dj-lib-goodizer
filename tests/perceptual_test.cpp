#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

#include "djcore/audio/PcmBuffer.h"
#include "djcore/model/Enums.h"
#include "djcore/processing/PerceptualOps.h"

using namespace djcore;

namespace {
constexpr double kPi = 3.14159265358979323846;

double correlation(const PcmBuffer& b) {
  double lr = 0, l2 = 0, r2 = 0;
  for (std::size_t i = 0; i < b.frames(); ++i) {
    lr += b.channel(0)[i] * b.channel(1)[i];
    l2 += b.channel(0)[i] * b.channel(0)[i];
    r2 += b.channel(1)[i] * b.channel(1)[i];
  }
  return lr / std::sqrt(l2 * r2);
}

}  // namespace

TEST(Perceptual, OpsAreTaggedPerceptualAltering) {
  CompressionOperation comp({});
  WidthBalanceOperation wb(1.5);
  EXPECT_EQ(comp.category(), OperationCategory::PerceptualAltering);
  EXPECT_EQ(wb.category(), OperationCategory::PerceptualAltering);
}

TEST(Perceptual, CompressionReducesDynamicRange) {
  // A compressor's defining property: it attenuates loud material more than
  // quiet material, shrinking the level gap between them. Use two sustained
  // tones so the envelope fully settles (no attack overshoot).
  const auto tone = [](double amp) {
    PcmBuffer b(1, 44100, 44100);
    for (std::size_t i = 0; i < b.frames(); ++i) {
      b.channel(0)[i] = static_cast<float>(amp * std::sin(2.0 * kPi * 200.0 * i / 44100.0));
    }
    return b;
  };
  const auto rms = [](const PcmBuffer& b) {
    double ss = 0.0;
    for (std::size_t i = 0; i < b.frames(); ++i) ss += b.channel(0)[i] * b.channel(0)[i];
    return std::sqrt(ss / b.frames());
  };

  PcmBuffer loud = tone(0.8);    // -1.9 dB, well above threshold
  PcmBuffer quiet = tone(0.1);   // -20 dB, at threshold => little reduction
  const double ratioBefore = rms(loud) / rms(quiet);

  const CompressorSettings s{/*threshold*/ -20.0, /*ratio*/ 8.0, 5.0, 60.0, /*makeup*/ 0.0};
  CompressionOperation(s).apply(loud);
  CompressionOperation(s).apply(quiet);
  const double ratioAfter = rms(loud) / rms(quiet);

  EXPECT_LT(ratioAfter, ratioBefore * 0.6);  // dynamic range meaningfully reduced
}

TEST(Perceptual, WidthFactorIncreasesStereoWidth) {
  PcmBuffer buf(2, 44100, 44100);
  for (std::size_t i = 0; i < buf.frames(); ++i) {
    const double t = static_cast<double>(i) / 44100.0;
    buf.channel(0)[i] = static_cast<float>(0.5 * std::sin(2.0 * kPi * 440.0 * t));
    buf.channel(1)[i] = static_cast<float>(0.5 * std::sin(2.0 * kPi * 445.0 * t));  // slightly decorrelated
  }
  const auto sideEnergy = [](const PcmBuffer& b) {
    double s = 0;
    for (std::size_t i = 0; i < b.frames(); ++i) {
      const double side = 0.5 * (b.channel(0)[i] - b.channel(1)[i]);
      s += side * side;
    }
    return s;
  };
  const double before = sideEnergy(buf);
  WidthBalanceOperation(2.0).apply(buf);
  EXPECT_GT(sideEnergy(buf), before * 1.5);
}

TEST(Perceptual, BalanceCorrectionReducesSkew) {
  PcmBuffer buf(2, 44100, 22050);
  const double gainR = std::pow(10.0, 3.0 / 20.0);  // right +3 dB
  for (std::size_t i = 0; i < buf.frames(); ++i) {
    const double v = 0.3 * std::sin(2.0 * kPi * 440.0 * i / 44100.0);
    buf.channel(0)[i] = static_cast<float>(v);
    buf.channel(1)[i] = static_cast<float>(v * gainR);
  }
  const auto balanceDb = [](const PcmBuffer& b) {
    double l2 = 0, r2 = 0;
    for (std::size_t i = 0; i < b.frames(); ++i) {
      l2 += b.channel(0)[i] * b.channel(0)[i];
      r2 += b.channel(1)[i] * b.channel(1)[i];
    }
    return 10.0 * std::log10(r2 / l2);
  };
  EXPECT_NEAR(balanceDb(buf), 3.0, 0.2);
  WidthBalanceOperation(1.0, /*correction*/ -3.0).apply(buf);  // cut right / boost left
  EXPECT_NEAR(balanceDb(buf), 0.0, 0.3);
}

TEST(Perceptual, MonoCrossCheckDetectsWideningHazard) {
  // A decorrelated stereo signal whose mono-safety drops as it is widened.
  PcmBuffer buf(2, 44100, 44100);
  for (std::size_t i = 0; i < buf.frames(); ++i) {
    const double t = static_cast<double>(i) / 44100.0;
    buf.channel(0)[i] = static_cast<float>(0.5 * std::sin(2.0 * kPi * 440.0 * t));
    buf.channel(1)[i] = static_cast<float>(0.5 * std::sin(2.0 * kPi * 440.0 * t + 1.0));  // phase-shifted
  }
  const double base = correlation(buf);
  const double widened = predictMonoCorrelationAfterWidth(buf, 2.5);
  EXPECT_LT(widened, base);  // widening worsens mono correlation

  const double monoThreshold = 0.3;
  const bool wouldHarm = widened < monoThreshold;
  EXPECT_TRUE(wouldHarm);  // cross-check would warn before committing
}
