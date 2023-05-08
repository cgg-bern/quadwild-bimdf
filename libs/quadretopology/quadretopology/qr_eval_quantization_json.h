#pragma once
#include "qr_eval_quantization.h"
#include <nlohmann/json.hpp>

namespace QuadRetopology {

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(QuantizationEvaluation,
    total_singularity_pairs,
    aligned_singularity_pairs,
    n_regular_charts,
    n_charts,
    isometry_term,
    side_isometry_term,
    regularity_term,
    regularity_term_v3,
    regularity_term_v4,
    regularity_term_v5,
    regularity_term_v6,
    alignment_term)

} // namespace QuadRetopology
