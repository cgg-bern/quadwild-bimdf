#pragma once

#include "includes/qr_charts.h"
#include "includes/qr_parameters.h"
#include "qr_singularity_pairs.h"
#include <limits>
#include <cstddef>
#include <ostream>

namespace QuadRetopology {

struct QuantizationEvaluation
{
    size_t total_singularity_pairs = std::numeric_limits<size_t>::max();
    size_t aligned_singularity_pairs = std::numeric_limits<size_t>::max();

    size_t n_regular_charts = std::numeric_limits<size_t>::max();
    size_t n_charts = std::numeric_limits<size_t>::max();

    double isometry_term = std::numeric_limits<double>::quiet_NaN();
    double side_isometry_term = std::numeric_limits<double>::quiet_NaN();

    /// regularity term without singularity alignment costs
    double regularity_term = std::numeric_limits<double>::quiet_NaN();
    /// split up for different valences:
    double regularity_term_v3 = std::numeric_limits<double>::quiet_NaN();
    double regularity_term_v4 = std::numeric_limits<double>::quiet_NaN();
    double regularity_term_v5 = std::numeric_limits<double>::quiet_NaN();
    double regularity_term_v6 = std::numeric_limits<double>::quiet_NaN();

    double alignment_term = std::numeric_limits<double>::quiet_NaN();
};

QuantizationEvaluation evaluate_quantization(
        const ChartData &chart_data,
        const std::vector<double> &chart_edge_length,
        const Parameters & parameters,
        const std::vector<int> &subside_lengths);

int chart_irregularity(
        const ChartData &chart_data,
        const std::vector<int> &subside_lengths,
        size_t chart_id);

bool is_singularity_pair_aligned(
        const ChartData &chart_data,
        const std::vector<int> &subside_lengths,
        SingularityPair const&pair);

std::ostream& operator<<(std::ostream &s, QuantizationEvaluation const &qe);

} // namespace QuadRetopology
