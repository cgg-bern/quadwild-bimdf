#pragma once

#include <cstddef>
#include <array>
#include <vector>
#include <optional>

namespace QuadRetopology {

struct ChartData;

using ChartId = std::size_t;

struct DirectedQuad {
    ChartId chart;
    std::array<int, 2> side_idx;
};

struct SingularityPair {
    std::array<ChartId, 2> charts;
    std::array<int, 2> side_idx;
    std::vector<DirectedQuad> quads;
};

#if 0
struct SidePair {
    SidePair (int _side, size_t _pair_idx)
        : side_idx(_side)
        , pair_idx(_pair_idx)
    {}
    int side_idx; // index in chart.sides
    size_t pair_idx; // index in SingularityPairInfo.pairs
};
using PairsPerChart = std::vector<SidePair>;
#endif
using PairedSides = std::array<bool, 6>; // maximum valence, last entries unused for lower valence patches

struct SingularityPairInfo {
    std::vector<SingularityPair> pairs;
    std::vector<PairedSides> paired_sides; // index with chart_id
    //void remove_pairs_with_irregular_charts(ChartData chart_data, std::vector<bool> &satisfied_regularity);
    void remove_unaligned_pairs(const std::vector<bool> &satisfied_alignment);
};

/// This code is adapted from qr_ilp.cpp.
/// TODO TIDY: move this to extra file and refactor qr_ilp to use it.
SingularityPairInfo find_singularity_pairs(const ChartData& chart_data);

} // namespace QuadRetopology
