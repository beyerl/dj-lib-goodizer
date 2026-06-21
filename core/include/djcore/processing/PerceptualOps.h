#pragma once

#include "djcore/audio/PcmBuffer.h"
#include "djcore/model/Enums.h"
#include "djcore/processing/IOperation.h"

namespace djcore {

// --- Opt-in, perceptual-altering operations (plan §3, FR-DR-6 / FR-WID-6) ----
// These change perceived dynamics or stereo image and are NEVER part of the
// default standardizing chain. They are categorized PerceptualAltering and must
// be explicitly enabled and confirmed by the user.

// Simple linked-stereo feed-forward compressor. Reduces dynamic range (lowers
// crest factor) — the opposite of gain-only normalization.
struct CompressorSettings {
  double thresholdDb = -18.0;
  double ratio = 4.0;
  double attackMs = 10.0;
  double releaseMs = 150.0;
  double makeupDb = 0.0;
};

class CompressionOperation : public IOperation {
 public:
  explicit CompressionOperation(CompressorSettings s) : s_(s) {}
  const char* name() const override { return "compression"; }
  OperationType type() const override { return OperationType::Compression; }
  OperationCategory category() const override { return OperationCategory::PerceptualAltering; }
  void apply(PcmBuffer& buffer) override;

 private:
  CompressorSettings s_;
};

// Mid/side stereo-width scaling plus L/R balance correction. widthFactor > 1
// widens, < 1 narrows; balanceCorrectionDb shifts the R-L balance by that many
// dB (e.g. -3 cuts right / boosts left to correct a +3 dB right-heavy skew).
class WidthBalanceOperation : public IOperation {
 public:
  WidthBalanceOperation(double widthFactor, double balanceCorrectionDb = 0.0)
      : widthFactor_(widthFactor), balanceCorrectionDb_(balanceCorrectionDb) {}
  const char* name() const override { return "width-balance"; }
  OperationType type() const override { return OperationType::WidthBalanceCorrection; }
  OperationCategory category() const override { return OperationCategory::PerceptualAltering; }
  void apply(PcmBuffer& buffer) override;

 private:
  double widthFactor_;
  double balanceCorrectionDb_;
};

// Mono-safety cross-check (FR-WID-7, AC 3.5.3): returns the inter-channel
// correlation the buffer would have after applying the given width factor.
// Callers warn the user when this would fall below the profile's mono threshold.
double predictMonoCorrelationAfterWidth(const PcmBuffer& buffer, double widthFactor);

}  // namespace djcore
