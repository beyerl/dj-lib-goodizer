#include "djcore/model/Enums.h"

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

}  // namespace djcore
