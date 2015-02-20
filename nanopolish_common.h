//---------------------------------------------------------
// Copyright 2015 Ontario Institute for Cancer Research
// Written by Jared Simpson (jared.simpson@oicr.on.ca)
//---------------------------------------------------------
//
// nanopolish_common -- Data structures and definitions
// shared across files
//
#ifndef NANOPOLISH_COMMON_H
#define NANOPOLISH_COMMON_H

#include <stdint.h>
#include <string>
#include <math.h>
#include "nanopolish_khmm_parameters.h"
#include "nanopolish_poremodel.h"
#include "profiler.h"

//
// Enumerated types
//
enum AlignmentPolicy
{
    AP_GLOBAL,
    AP_SEMI_KMER
};

//
// Constants
//
const uint8_t K = 5;

// A table to map { A, C, G, T } => { 0, 1, 2, 3 }
static const uint8_t base_rank[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,1,0,0,0,2,0,0,0,0,0,0,0,0,
    0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

//
// Data structures
//

// The raw event data for a read
struct EventSequence
{
    uint32_t n_events;
    const double* level;
    const double* stdv;
    const double* time;
};

//
struct SquiggleRead
{
    // unique identifier of the read
    uint32_t read_id;

    // one model for each strand
    PoreModel pore_model[2];

    // one event sequence for each strand
    EventSequence events[2];

    // one set of parameters per strand
    KHMMParameters parameters[2];
};

// This struct is used as input into the HMM
// It tracks where the event stream starts/stops
// for the partial consensus sequence under consideration
struct HMMConsReadState
{
    SquiggleRead* read;
    uint32_t anchor_index;
    uint32_t event_start_idx;
    uint32_t event_stop_idx;
    uint8_t strand;
    int8_t stride;
    uint8_t rc;
    std::string alignment;
};

struct PosteriorState
{
    uint32_t event_idx;
    uint32_t kmer_idx;
    double l_posterior;
    double l_fm;
    double log_transition_probability;
    char state;
};

//
// Functions
//

//
// Kmer Ranks
//
inline uint32_t kmer_rank(const char* str, uint32_t K)
{
    uint32_t rank = 0;
    for(uint32_t i = 0; i < K; ++i)
        rank |= base_rank[str[i]] << 2 * (K - i - 1);
    return rank;
}

inline uint32_t rc_kmer_rank(const char* str, uint32_t K)
{
    uint32_t rank = 0;
    for(int32_t i = K - 1; i >= 0; --i)
        rank |= ((3 - base_rank[str[i]]) << 2 * i);
    return rank;
}

// wrapper to get the rank for a kmer on the correct strand wrt to the read state
inline uint32_t get_rank(const HMMConsReadState& state, const char* s, uint32_t ki)
{
    const char* p = s + ki;
    return !state.rc ?  kmer_rank(p, K) : rc_kmer_rank(p, K);
}

// Add the log-scaled values a and b using a transform to avoid precision errors
inline double add_logs(const double a, const double b)
{
    if(a == -INFINITY && b == -INFINITY)
        return -INFINITY;

    if(a > b) {
        double diff = b - a;
        return a + log(1.0 + exp(diff));
    } else {
        double diff = a - b;
        return b + log(1.0 + exp(diff));
    }
}

// Make a unique index for the strand this read state represents
inline uint32_t get_strand_idx(const HMMConsReadState& rs)
{
    return rs.read->read_id + rs.strand;
}

// Get the duration of the given event
double get_duration(const SquiggleRead& read, uint32_t event_idx, uint32_t strand);

// Correct the current level observed for the given event by the drift factor
double get_drift_corrected_level(const SquiggleRead& read, uint32_t event_idx, uint32_t strand);

// Increment the input string to be the next DNA sequence in lexicographic order
void lexicographic_next(std::string& str);

#endif
