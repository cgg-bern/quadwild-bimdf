#include "qr_singularity_pairs.h"
#include "includes/qr_charts.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace QuadRetopology {

static std::optional<SingularityPair>
trace_to_partner(const ChartData &chart_data,
                 ChartId chart_id,
                 int side_idx)
{
#if 0
    // just for debug
    return {};
#endif
    SingularityPair sp;
    sp.charts[0] = chart_id;
    sp.side_idx[0] = side_idx;

    size_t inc_valence;
    size_t inc_chart_id;
    int inc_side_idx;

    do {
        const Chart& chart = chart_data.charts.at(chart_id);
        const ChartSide& side = chart.chartSides.at(side_idx);

        if (side.subsides.size() != 1) {
            return {};
        }
        auto subside_id = side.subsides.at(0);
        const ChartSubside &subside = chart_data.subsides.at(subside_id);
        size_t incident_idx = static_cast<size_t>(subside.incidentCharts[0]) == chart_id ? 1 : 0;
        assert(static_cast<size_t>(subside.incidentCharts[1-incident_idx]) == chart_id);

        inc_chart_id = static_cast<size_t>(subside.incidentCharts[incident_idx]);
        if (inc_chart_id == static_cast<size_t>(-1)) { // boundary
            return {};
        }
        inc_side_idx = subside.incidentChartSideId[incident_idx];
        const Chart& inc_chart = chart_data.charts[inc_chart_id];
        const ChartSide& inc_side = inc_chart.chartSides[inc_side_idx];
        if (inc_side.subsides.size() != 1) {
            return {};
        }
        inc_valence = inc_chart.chartSides.size();
        if (inc_valence == 4) {
            chart_id = inc_chart_id;
            side_idx = (inc_side_idx + 2) % 4;
            sp.quads.push_back({inc_chart_id, {inc_side_idx, side_idx}});
        }
    } while (inc_valence == 4);

    if (inc_valence != 3 && inc_valence != 5 && inc_valence != 6) {
        throw std::runtime_error("weird patch valence: " + std::to_string(inc_valence));
    }

    // we arrived at a suitable chart!
    sp.charts[1] = inc_chart_id;
    sp.side_idx[1] = inc_side_idx;

    if (sp.charts[1] == sp.charts[0]) {
#if 1
        // self-pairing.
        // TODO: handle this (does original code handle this somewhere?)
        return {};
#endif
    }
    if (sp.charts[1] < sp.charts[0]) {
        // ignore: we have already found the path in the other direction.
        return {};
    }


    return sp;
}

SingularityPairInfo find_singularity_pairs(const ChartData &chart_data)
{
    SingularityPairInfo result;

    PairedSides all_false; all_false.fill(false);
    result.paired_sides.resize(0);
    result.paired_sides.resize(chart_data.charts.size(), all_false);

    // TODO: filter pairs based on constraint satisfaction as in ILP
    for (size_t chart_id = 0; chart_id < chart_data.charts.size(); ++chart_id)
    {
        const Chart& chart = chart_data.charts.at(chart_id);
        const size_t valence = chart.chartSides.size();
        if (valence != 3 && valence != 5 && valence != 6) {
            continue;
        }
        for (size_t side_idx = 0; side_idx < valence; side_idx++) {
            auto maybe_pair = trace_to_partner(chart_data, chart_id, side_idx);
            if (maybe_pair.has_value()) {
                result.pairs.push_back(*maybe_pair);
                for (int i = 0; i < 2; ++i) {
                    result.paired_sides[maybe_pair->charts[i]].at(maybe_pair->side_idx[i]) = true;
                }
                for (const auto &quad: maybe_pair->quads) {
                    for (int i = 0; i < 2; ++i) {
                        result.paired_sides[quad.chart].at(quad.side_idx[i]) = true;
                    }
                }
            }
        }
    }
    return result;
}

#if 0
void SingularityPairInfo::remove_pairs_with_irregular_charts(
        ChartData chart_data,
        std::vector<bool> &satisfied_regularity)
{

    // TODO
}
#endif

void SingularityPairInfo::remove_unaligned_pairs(const std::vector<bool> &satisfied_alignment)
{
    assert(satisfied_alignment.size() == pairs.size());
    std::vector<SingularityPair> new_pairs;
    for (size_t pair_id = 0; pair_id < pairs.size(); ++pair_id)
    {
        const auto &pair = pairs.at(pair_id);
        if (!satisfied_alignment[pair_id])
        {
            // remove relevant paired_sides
            paired_sides[pair.charts[0]][pair.side_idx[0]] = false;
            paired_sides[pair.charts[1]][pair.side_idx[1]] = false;
            for (const auto &quad: pair.quads) {
                for (int i = 0; i < 2; ++i) {
                    paired_sides[quad.chart].at(quad.side_idx[i]) = false;
                }
            }
        } else {
            new_pairs.push_back(pair);
        }
    }

    pairs = std::move(new_pairs);
}

} // namespace QuadRetopology
