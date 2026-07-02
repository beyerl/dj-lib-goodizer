#include "djcore/model/Enums.h"

#include <string>

namespace djcore {

const char* toString(OperationType type) {
  switch (type) {
    case OperationType::None:
      return "none";
    case OperationType::ResampleTranscode:
      return "resample_transcode";
    case OperationType::Dither:
      return "dither";
    case OperationType::SilenceTrim:
      return "silence_trim";
    case OperationType::GainNormalize:
      return "gain_normalize";
    case OperationType::Compression:
      return "compression";
    case OperationType::WidthBalanceCorrection:
      return "width_balance_correction";
  }
  return "unknown";
}

OperationType operationFromString(const std::string& s) {
  if (s == "resample_transcode") return OperationType::ResampleTranscode;
  if (s == "dither") return OperationType::Dither;
  if (s == "silence_trim") return OperationType::SilenceTrim;
  if (s == "gain_normalize") return OperationType::GainNormalize;
  if (s == "compression") return OperationType::Compression;
  if (s == "width_balance_correction") return OperationType::WidthBalanceCorrection;
  return OperationType::None;
}

}  // namespace djcore
