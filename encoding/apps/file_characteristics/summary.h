#ifndef SUMMARY_H
#define SUMMARY_H

#include "statistics.h"

typedef struct {
  double informationBits;
  double informationBytes;
  long compressedTextMinSize;
  long archiveLengthWithNonStandardFrequenciesTable;
  long archiveLengthWithNormalizedFrequenciesTable;

} Summary;

Summary countSummary(Statistics statistics);

#endif
