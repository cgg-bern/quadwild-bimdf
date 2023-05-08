#include "qr_eval_quantization.h"
#include "qr_singularity_pairs.h"
#include <iostream>
#include <cassert>

namespace QuadRetopology {
/// for charts of valence < 6, the last entries are unused
using SideLengths = std::array<double, 6>;

SideLengths get_side_lengths(
        const ChartData &chart_data,
        const std::vector<int> &subside_lengths,
        size_t chart_id)
{
    std::array<double, 6> side_lengths;
    const Chart& chart = chart_data.charts.at(chart_id);

    size_t valence = chart.chartSides.size();
    for (size_t side_idx = 0; side_idx < valence; ++side_idx)
    {
        const auto &side = chart.chartSides[side_idx];
        double sum = 0;
        for (const auto subside_idx: side.subsides) {
            sum += subside_lengths[subside_idx];
        }
        side_lengths[side_idx] = sum;
    }
    return side_lengths;
}

/// based on qr_ilp code
std::array<int, 2> get_up_down(
        size_t valence,
        size_t side_idx,
        SideLengths &lens)
{
    int up = 0, down = 0;

    if (valence == 3) {
        const int subside0Sum = lens[side_idx];
        const int subside1Sum = lens[(side_idx+1)%valence];
        const int subside2Sum = lens[(side_idx+2)%valence];

        down = subside0Sum + subside2Sum - subside1Sum;
        up = subside1Sum + subside0Sum - subside2Sum;
    }
    //Singularity alignment for pentagonal case
    else if (valence == 5) {
        const int subside0Sum = lens[side_idx];
        const int subside1Sum = lens[(side_idx+1)%valence];
        const int subside2Sum = lens[(side_idx+2)%valence];
        const int subside3Sum = lens[(side_idx+3)%valence];
        const int subside4Sum = lens[(side_idx+4)%valence];

        // NOTE: figure 16 formula in quadwild paper is wrong! refer to tarini instead (as in qr_ilp)
        down = (subside0Sum + subside1Sum + subside2Sum) - (subside3Sum + subside4Sum);
        up = (subside3Sum + subside4Sum + subside0Sum) - (subside1Sum + subside2Sum);
    }
    //Singularity alignment for hexagonal case
    else if (valence == 6) {
        const int subside0Sum = lens[side_idx];
        const int subside2Sum = lens[(side_idx+2)%valence];
        const int subside4Sum = lens[(side_idx+4)%valence];

        down = subside0Sum + subside2Sum - subside4Sum;
        up = subside4Sum + subside0Sum - subside2Sum;
    } else {
        assert(false);
    }
    return {up, down};
}

std::array<std::array<int, 2>, 2> get_up_down(
        const ChartData &chart_data,
        const std::vector<int> &subside_lengths,
        SingularityPair const&pair)
{
    std::array<std::array<int, 2>, 2> up_down;

    for(int i = 0; i < 2; ++i) {
        int valence = chart_data.charts[pair.charts[i]].chartSides.size();
        auto lengths = get_side_lengths(chart_data, subside_lengths, pair.charts[i]);
        up_down[i] = get_up_down(valence, pair.side_idx[i], lengths);
    }
    return up_down;
}

/// adapted from qr_ilp
bool is_singularity_pair_aligned(
        const ChartData &chart_data,
        const std::vector<int> &subside_lengths,
        SingularityPair const&pair)
{
    // TODO: first check if charts are semi-regular!
    auto up_down = get_up_down(chart_data, subside_lengths, pair);
    // TODO TODO TODO: check intermediate quads fro regular quant!!!

    bool aligned = true;
    if (up_down[0][1] != up_down[1][0]) {
        aligned = false;
    }
    if (up_down[0][0] != up_down[1][1]) {
        aligned = false;
    }
    if (!aligned)
    {
#if 0
        std::cout << "\tunaligned chart pair:\n";
                  << "chart " << pair.charts[0] << "(s " << pair.side_idx[0] << "), "
                  << "chart " << pair.charts[1] << "(s " << pair.side_idx[1] << "): "
                  << up_down[0][0] << ", "
                  << up_down[0][1] << " <=> "
                  << up_down[1][0] << ", "
                  << up_down[1][1] << std::endl;
        for(int i = 0; i < 2; ++i) {
            int valence = chart_data.charts[pair.charts[i]].chartSides.size();
            auto lengths = get_side_lengths(chart_data, subside_lengths, pair.charts[i]);
            up_down[i] = get_up_down(valence, pair.side_idx[i], lengths);
            std::cout << "\t\tchart " << pair.charts[i]
                      << "(" << pair.side_idx[i] << ")"
                      << ": " << up_down[i][0] << ", " << up_down[i][1] << "; sides: ";
            for(int j =0; j < valence; ++j) {
                std::cout << lengths[j] << " ";
            }
            std::cout << std::endl;
        }
#endif
    }
    return aligned;
}

int chart_irregularity(
        const ChartData &chart_data,
        const std::vector<int> &subside_lengths,
        size_t chart_id)
{
    std::array<double, 6> side_lengths = get_side_lengths(chart_data, subside_lengths, chart_id);
    const Chart& chart = chart_data.charts.at(chart_id);

    size_t valence = chart.chartSides.size();

    int value = 0;
    // adapted from qr_ilp:
    size_t j = 0;
    for (; j < valence; j++) {

        //Regularity for quad case
        if (valence == 4) {
            int subside0Sum = side_lengths[j];
            int subside2Sum = side_lengths[(j+2)%valence];

            value += std::abs(subside0Sum - subside2Sum);
        }
        //Regularity for triangular case
        else if (valence == 3) {
            int subside0Sum = side_lengths[j];
            int subside1Sum = side_lengths[(j+1)%valence];
            int subside2Sum = side_lengths[(j+2)%valence];

            int c = subside0Sum + 1 - subside1Sum - subside2Sum;
            if (c > 0) {
                value += c;
            }
        }
        //Regularity for pentagonal case
        else if (valence == 5) {
            int subside0Sum = side_lengths[j];
            int subside1Sum = side_lengths[(j+1)%valence];
            int subside2Sum = side_lengths[(j+2)%valence];
            int subside3Sum = side_lengths[(j+3)%valence];
            int subside4Sum = side_lengths[(j+4)%valence];

            int c = subside0Sum + subside1Sum + 1 - subside2Sum - subside3Sum - subside4Sum;
            if (c > 0) {
                value += c;
            }
        }
        //Regularity for hexagonal case
        else if (valence == 6) {
            int subside0Sum = side_lengths[j];
            int subside2Sum = side_lengths[(j+2)%valence];
            int subside4Sum = side_lengths[(j+4)%valence];

            int c =  subside0Sum + 1 - subside2Sum - subside4Sum;


            int parityEquation = subside0Sum + subside2Sum + subside4Sum;
#if 0
            if (parityEquation < 5) { // should 3 not be enough!??!!?
                std::cout << "ERROR: hex chart " << chart_id << ", side " << j << " parity too low:" << parityEquation << std::endl;
            }
#endif
            int hexFree = parityEquation & 1;
            if (false && hexFree) {
                std::cout << "hex chart " << chart_id
                          << ", side " << j
                          << " parity violation: " << parityEquation
                          << " (c = " << c << ")"
                           << std::endl;
                //throw std::runtime_error("hex chart parity violated");
            }
            value += hexFree;
            if (c > 0) {
                value += c;
            }
        } else {
            assert(false);
        }
    }

    if (0) { //  debug  irregular charts?
        if (value != 0) {
            std::cout << "\tchart " << chart_id
                      << " (val. " << valence
                      << ") side " << j
                      << " irregularity: " << value;
            std::cout << "\n\t\t side lengths: ";
            for (size_t i = 0; i < valence; ++i) {
                std::cout << side_lengths[i] << " ";
            }
            std::cout << "\n\t\t subside lengths: ";
            for (const auto subside_idx: chart.chartSubsides) {
                std::cout << subside_lengths[subside_idx] << " ";
            }
            std::cout << std::endl;
        }
    }
    return value;
}

/// Goal: evaluate costs as in qr_ilp with no dropped constraints
QuantizationEvaluation evaluate_quantization(
        const ChartData &chart_data,
        const std::vector<double> &chart_edge_length,
        const Parameters &parameters,
        const std::vector<int> &subside_lengths)
{
    const double isoWeight = parameters.alpha;
    const double regWeight = (1 - parameters.alpha);
    QuantizationEvaluation result;
    result.n_charts = chart_data.charts.size();
    result.n_regular_charts = 0;
    result.isometry_term = 0;
    result.regularity_term = 0;
    result.regularity_term_v3 = 0;
    result.regularity_term_v4 = 0;
    result.regularity_term_v5 = 0;
    result.regularity_term_v6 = 0;
    result.alignment_term = 0;
    result.side_isometry_term = 0;
    std::array<double, 7> irreg_per_val = {0};
    for (size_t chart_id = 0; chart_id < chart_data.charts.size(); ++chart_id)
    {
        const Chart& chart = chart_data.charts.at(chart_id);
        const size_t valence = chart.chartSides.size();
        const auto cur_chart_edge_len = chart_edge_length[chart_id];

        double chart_isometry = 0;
        for (const auto subside_id: chart.chartSubsides) {
            const ChartSubside& subside = chart_data.subsides.at(subside_id);
            double target = std::max(1., subside.length / cur_chart_edge_len);
            double deviation = target - double(subside_lengths.at(subside_id));
            chart_isometry += deviation * deviation;
        }
        result.isometry_term += isoWeight * chart_isometry / chart.chartSubsides.size();
        // note: isometry term  matches the ILP formulation in my tests

        double side_isometry = 0;
        for (const auto &side: chart.chartSides) {
            double target = side.length / cur_chart_edge_len;
            double len = 0.;
            for (const auto subside_idx: side.subsides) {
                len += subside_lengths[subside_idx];
            }
            double deviation = target - len;
            side_isometry += deviation * deviation;
        }
        result.side_isometry_term += side_isometry;

        if (!parameters.regularityQuadrilaterals && valence == 4) {
            continue;
        }
        if (!parameters.regularityNonQuadrilaterals && valence != 4) {
            continue;
        }
        int irreg_int = chart_irregularity(chart_data, subside_lengths, chart_id);
        assert (irreg_int >= 0);
        if (irreg_int == 0) {
            ++result.n_regular_charts;
        }
        double irreg_cost = irreg_int;
        //
        if (valence == 6) {
            irreg_cost *= .5; // ILP uses "(c+hexFree)/2.0" cost
        }
        if (valence != 4) {
            irreg_cost *= parameters.regularityNonQuadrilateralsWeight;
        }
        //size_t numRegularityTerms = valence;
        double weighted_irreg_cost = irreg_cost * regWeight / (valence * valence);
        result.regularity_term += weighted_irreg_cost;
        if (valence == 3) {
            result.regularity_term_v3 += weighted_irreg_cost;
        } else if (valence == 4) {
            result.regularity_term_v4 += weighted_irreg_cost;
        } else if (valence == 5) {
            result.regularity_term_v5 += weighted_irreg_cost;
        } else if (valence == 6) {
            result.regularity_term_v6 += weighted_irreg_cost;
        }
        irreg_per_val[valence] += irreg_cost * regWeight / (valence * valence);
        // TODO alignment term
    }
    std::cout << "\n\nirreg per val:"
              << "tri:    " << irreg_per_val[3]
              << ",quad:  " << irreg_per_val[4]
              << ",penta: " << irreg_per_val[5]
              << ",hex: " << irreg_per_val[6]
              << "\n" << std::endl;
    SingularityPairInfo spi = find_singularity_pairs(chart_data);

    result.total_singularity_pairs = spi.pairs.size();
    result.aligned_singularity_pairs = 0;

    for(const auto &pair: spi.pairs) {
        //std::cout << "considering alignment pair " << pair.charts[0] << " - " << pair.charts[1] << std::endl;
#if 0
        if (pair.charts[0] == pair.charts[1]) {
            // ILP skips this case?!?! copy that behaviour?
            continue;
        }
#endif
        auto up_down = get_up_down(chart_data, subside_lengths, pair);

        int value1 = std::abs(up_down[0][0] - up_down[1][1]);
        int value2 = std::abs(up_down[0][1] - up_down[1][0]);
        double cost = regWeight * parameters.alignSingularitiesWeight *
                    .5 * (value1 + value2);
        for (const ChartId chart_id: pair.charts) {
            const Chart& chart = chart_data.charts.at(chart_id);
            const size_t valence = chart.chartSides.size();
            result.alignment_term += cost / (valence * valence);

        }
        if (value1 + value2 == 0) {
        //if (is_singularity_pair_aligned(chart_data, subside_lengths, pair)) {
            ++result.aligned_singularity_pairs;
        }

    }

    return result;
}

std::ostream &operator<<(std::ostream &s, const QuantizationEvaluation &qe)
{
    s << "\tSing. alignment: "
      << qe.aligned_singularity_pairs << " / " <<  qe.total_singularity_pairs << "\n"
      << "\tRegularity: " << qe.n_regular_charts << " / " <<  qe.n_charts<< "\n"
      << "\tIsometry term:   " << qe.isometry_term << "\n"
      << "\tRegularity term: " << qe.regularity_term << "\n"
      << "\tAlignment term: " << qe.alignment_term << "\n"
      << "\tIso + Reg term:  " << qe.isometry_term + qe.regularity_term << "\n"
      << "additional metrics not used in ilp:\n"
      << "\tSide iso term:   " << qe.side_isometry_term << "\n"
      << std::endl;
    return s;
}

} // namespace QuadRetopology
