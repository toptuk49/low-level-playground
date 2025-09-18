#include <math.h>

#include "summary.h"

Summary initSummary();

Summary countSummary(Statistics statistics) {
  Summary summary = initSummary();

  for (int i = 0; i < ALPHABET_SIZE; i++) {
    summary.informationBits +=
        statistics.byteCounts[i] * statistics.byteInformationAmounts[i];
  }
  summary.informationBytes = summary.informationBits / 8.0;

  summary.compressedTextMinSize = (long)ceil(summary.informationBytes);
  summary.archiveLengthWithNonStandardFrequenciesTable =
      summary.compressedTextMinSize + 256 * 8;
  summary.archiveLengthWithNormalizedFrequenciesTable =
      summary.compressedTextMinSize + 256 * 1;

  return summary;
}

Summary initSummary() {
  Summary s = {
      .informationBits = 0.0,
  };

  return s;
}
