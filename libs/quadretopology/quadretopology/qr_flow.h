#pragma once

#include "includes/qr_charts.h"
#include "includes/qr_parameters.h"
#include <libTimekeeper/StopWatch.hh>
#include <libsatsuma/Extra/Highlevel.hh>
#include <nlohmann/json.hpp>

namespace QuadRetopology {

struct FlowStats {
    size_t n_pair_unaligners_used;
    double cost_iso_nonpaired;
    double cost_iso_paired;
    double cost_regularity_neighbor_v3;
    double cost_regularity_neighbor_v4;
    double cost_regularity_neighbor_v5;
    double cost_regularity_neighbor_v6;
    double cost_regularity_neighbor = cost_regularity_neighbor_v3
                                    + cost_regularity_neighbor_v4
                                    + cost_regularity_neighbor_v5
                                    + cost_regularity_neighbor_v6;

    double cost_regularity_opposite_v6;

    double cost_regularity_sideloops_v3;
    double cost_regularity_sideloops_v4;
    double cost_regularity_sideloops_v5;
    double cost_regularity_sideloops_v6;

    double cost_regularity_sideloops = cost_regularity_sideloops_v3
                                     + cost_regularity_sideloops_v4
                                     + cost_regularity_sideloops_v5
                                     + cost_regularity_sideloops_v6;
    double cost_sing_on_bound_v3;
    double cost_sing_on_bound_v5;
    double cost_sing_on_bound_v6;

    double cost_sing_on_bound = cost_sing_on_bound_v3
                              + cost_sing_on_bound_v5
                              + cost_sing_on_bound_v6;

    double cost_regularity = cost_regularity_neighbor
                           + cost_regularity_opposite_v6
                           + cost_regularity_sideloops
                           + cost_sing_on_bound;

    double cost_alignment;
    double cost_sum() const {
        return cost_iso_nonpaired
                + cost_iso_paired
                + cost_regularity
                + cost_alignment;
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FlowStats,
                                   n_pair_unaligners_used,
                                   cost_iso_nonpaired,
                                   cost_iso_paired,
                                   cost_regularity_neighbor_v3,
                                   cost_regularity_neighbor_v4,
                                   cost_regularity_neighbor_v5,
                                   cost_regularity_neighbor_v6,
                                   cost_regularity_neighbor,
                                   cost_regularity_opposite_v6,
                                   cost_regularity_sideloops_v3,
                                   cost_regularity_sideloops_v4,
                                   cost_regularity_sideloops_v5,
                                   cost_regularity_sideloops_v6,
                                   cost_regularity_sideloops,
                                   cost_sing_on_bound_v3,
                                   cost_sing_on_bound_v5,
                                   cost_sing_on_bound_v6,
                                   cost_sing_on_bound,
                                   cost_regularity,
                                   cost_alignment)


struct FlowResult {
    std::vector<Satsuma::BiMDFFullResult> bimdf_results;
    std::vector<FlowStats> stats;
    Timekeeper::HierarchicalStopWatchResult stopwatch;
};

FlowResult findSubdivisionsFlow(
        const ChartData& chartData,
        const std::vector<double>& chartEdgeLength,
        const Parameters& parameters,
        double& gap,
        std::vector<int>& results);
} // namespace QuadRetopology
