#ifndef SUBSTRINGS_MARKOV_MODEL_H
#define SUBSTRINGS_MARKOV_MODEL_H

#include "types.h"

#define ALPHABET_SIZE 256

typedef struct MarkovModel MarkovModel;

MarkovModel* markov_model_create(void);
void markov_model_destroy(MarkovModel* self);

Result markov_model_process_data(MarkovModel* self, const Byte* data,
                                 Size data_size);

uint64_t markov_model_get_pair_count(const MarkovModel* self, Byte first,
                                     Byte second);
uint64_t markov_model_get_prefix_count(const MarkovModel* self, Byte prefix);
uint64_t markov_model_get_symbol_count(const MarkovModel* self, Byte symbol);
double markov_model_get_conditional_probability(const MarkovModel* self,
                                                Byte first, Byte second);
double markov_model_get_information(const MarkovModel* self, Byte first,
                                    Byte second);

double markov_model_get_total_information_bits(const MarkovModel* self);
double markov_model_get_total_information_bytes(const MarkovModel* self);
uint64_t markov_model_get_total_pairs(const MarkovModel* self);

bool markov_model_validate_test_cases(void);

#endif  // SUBSTRINGS_MARKOV_MODEL_H
