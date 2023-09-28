/***************************************************************************/
/* Copyright(C) 2023 Martin Heistermann <martin.heistermann@unibe.ch>

Implementation described in
Min-Deviation-Flow in Bi-directed Graphs for T-Mesh Quantization, Martin Heistermann, Jethro Warnett, David Bommes, ACM Trans. Graph. 2023

https://www.algohex.eu/publications/bimdf-quantization/


All rights reserved.
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
****************************************************************************/

#include "qr_flow.h"
#include "qr_flow_config.h"
#include "qr_singularity_pairs.h"
#include "qr_eval_quantization.h"

#include <iostream>
#include <vector>
#include <memory>
#include <optional>

#include <libsatsuma/Problems/BiMDF.hh>
#include <libsatsuma/Extra/Highlevel.hh>
#include <libTimekeeper/StopWatchPrinting.hh>

namespace QuadRetopology {
using Satsuma::BiMDF;
using Node = Satsuma::BiMDF::Node;
using Edge = Satsuma::BiMDF::Edge;

struct FlowProblem {
    std::unique_ptr<Satsuma::BiMDF> bimdf = std::make_unique<Satsuma::BiMDF>();
    std::vector<std::array<Edge, 2>> subside_edges; // n_subsides-sized vector with corresponding bi-mdf edge(s)
    std::array<std::vector<Edge>,7> emergency_sideloops_per_valence;
    std::array<std::vector<Edge>,7> emergency_neighbor_per_valence;
    std::vector<Edge> emergency_opposite_v6;
    std::array<std::vector<Edge>,7> sing_on_bound_edges_per_valence;
    std::vector<Edge> unpaired_edges;
    std::vector<Edge> paired_edges;
    std::vector<Edge> pair_unaligners;
};

#define EMERGENCY 1

double side_length(
        const ChartData& chart_data,
        const ChartSide& side)
{
    double length = 0;
    for (const auto subside_id: side.subsides) {
        length += chart_data.subsides[subside_id].length;
    }
    return length;
}

struct TargetsAndWeight {
    std::array<double, 2> targets;
    double weight;
};

TargetsAndWeight virtual_subside_target(
        const ChartData& chart_data,
        const std::vector<double>& chart_edge_length,
        size_t chart_id,
        size_t side_idx)
{
    auto scale = 1. / chart_edge_length[chart_id];
    const Chart &chart = chart_data.charts[chart_id];
    size_t valence = chart.chartSides.size();
    const ChartSide side_a = chart.chartSides[side_idx];

    if (valence != 3) {
        // TODO: handle non-triangles better
        double half = scale * .5 * side_a.length;
        return {.targets = {half, half},
                .weight = (valence == 4) ? 0. : 1.};
    }
    //ChartSubside const& a_subside = chart_data.subsides[side_a.subsides[0]];
    // side_a.reversedSubside[0] ? a_subside.vertices.front() : a_subside.vertices.back();
    auto v0 = side_a.vertices.front();
    auto v1 = side_a.vertices.back();
    const ChartSide side_b = chart.chartSides[(side_idx+1) % valence];
    const ChartSide side_c = chart.chartSides[(side_idx+2) % valence];
    assert(side_b.vertices.front() == v1);
    assert(side_b.vertices.back() == side_c.vertices.front());
    assert(side_c.vertices.back() == v0);
    assert(side_a.subsides.size() == 1);

    double len_a = side_length(chart_data, side_a);
    double len_b = side_length(chart_data, side_b);
    double len_c = side_length(chart_data, side_c);
    double target = .5 * (len_a + len_b - len_c);
    if (target < 0) {
        target = 0;
    }
    if (target > len_a) {
        target = len_a;
    }
    return {.targets = {scale * target, scale * (len_a - target)}, .weight = 1.};
}

static FlowProblem make_bimdf(
        const FlowConfig& flow_config,
        const ChartData& chart_data,
        const std::vector<double>& chart_edge_length,
        const Parameters& parameters,
        const SingularityPairInfo &spi,
        /// empty or per-chart satisfaction
        const std::vector<bool> &satisfied_regularity,
        /// empty or per-pair satisfaction
        //const std::vector<bool> &satisfied_alignment
        const FlowProblem *previous_problem = nullptr,
        const Satsuma::BiMDF::Solution *previous_sol = nullptr
        )
{
    assert(satisfied_regularity.empty() || satisfied_regularity.size() == chart_data.charts.size());

    auto should_use_chart = [&](int id) -> bool {
        if (id == -1) { // boundary
            return true;
        }
        const Chart& chart = chart_data.charts.at(id);
        return chart.faces.size() > 0;
    };

    std::cout << "\nBi-MDF setup.\n";
    std::cout << "using " << spi.pairs.size() << " singularity pairs" << std::endl;

    FlowProblem problem;

    auto& bimdf = *problem.bimdf;
    auto &g = bimdf.g;
    problem.subside_edges.resize(chart_data.subsides.size(), {lemon::INVALID, lemon::INVALID});

    const double alpha = parameters.alpha; // 0.02 in default setup.txt
    const double isometry_weight = alpha;
    const double regularity_weight = 1 - alpha;
    //const double regularity_weight = .2 * (1 - alpha); // TODO JUST FOR DEBUG

    // 1 side node per side
    //      (connects internally to exactly one inner node,
    //       use that edge for cost function)
    //      edges from "outside" always have a *head* here
    // per chart:
    //   - connect inner nodes with free edges (except emergency nodes)
    //      quad: 2 inner nodes + 1 emergency node
    //      n-gon: n inner nodes + 1 emergency node
    //
    // 1 edge per subside: connect incident side nodes (free)
    //
    // side nodes aid simple implementation, but increase network size.
    //    TODO PERF: if a side has only 1 subside, we can skip the side node.
    //    alternatively, we might want to simplify using a postprocess in libsatsuma?

    // TODO: reserve emergency_neighbor_per_valence
    // TODO: reserve emergency_sideloops_per_valence
    // TODO: reserve sing_on_bound_edges_per_valence
    problem.unpaired_edges.reserve(chart_data.subsides.size()*2); // guess
    problem.paired_edges.reserve(4 * spi.paired_sides.size()); // guess, ignored quad edges

    auto add_subside_edge = [&](
            Node u, bool u_head,
            Node v, bool v_head,
            double target, double weight,
            ObjectiveKind obj = ObjectiveKind::QuadraticDeviation,
            int lower=1
            ) -> Edge
    {
        assert (u != lemon::INVALID);
        assert (v != lemon::INVALID);
        Satsuma::CostFunction::Function cf;
        if (obj == ObjectiveKind::QuadraticDeviation) {
            cf = Satsuma::CostFunction::QuadDeviation{.target = target, .weight = weight};
        } else if (obj == ObjectiveKind::AbsDeviation) {
            cf = Satsuma::CostFunction::AbsDeviation{.target = target, .weight = weight};
        } else {
            throw std::runtime_error("unknown objective kind");
        }
        return bimdf.add_edge({
                           .u = u, .v = v,
                           .u_head = u_head, .v_head = v_head,
                           .cost_function = cf,
                           .lower = lower, // subsides must be quantized to >= 1
                           .upper = BiMDF::inf()});
    };
    // TODO PERF:  g.reserveNode(..); g.reserveEdge(..);

    std::vector<std::vector<Node>> chart_side_nodes(chart_data.charts.size());
    std::vector<std::vector<std::array<Node,2>>> chart_side_node_pairs(chart_data.charts.size());

    size_t n_unused_charts = 0;
    for (size_t chart_id = 0; chart_id < chart_data.charts.size(); ++chart_id)
    {
        const Chart& chart = chart_data.charts.at(chart_id);
        if (!should_use_chart(chart_id)) {
            ++n_unused_charts;
            continue;
        }

        auto const &side_paired = spi.paired_sides[chart_id];

        const size_t valence = chart.chartSides.size();
        if (valence < 3 || valence > 6) {
            assert(false);
            throw std::runtime_error(std::string("valence not handled: ") + std::to_string(valence));
        }

        double chart_regularity_weight = regularity_weight / (valence*valence);
        //double chart_isometry_weight = isometry_weight / valence;
        if (valence != 4) {
            chart_regularity_weight *= parameters.regularityNonQuadrilateralsWeight;
        }

        if (!satisfied_regularity.empty()
                && !satisfied_regularity.at(chart_id)
                && ((parameters.repeatLosingConstraintsQuads && valence == 4)
                    || (parameters.repeatLosingConstraintsNonQuads && valence !=4)))
        {
                chart_regularity_weight = 0;
        }

        double emergency_side_loop_weight = 4 * chart_regularity_weight;
        double emergency_neighbour_weight = 4 * chart_regularity_weight;
        double singularity_on_boundary_weight = chart_regularity_weight;

        if (valence == 3) {
            emergency_side_loop_weight = 4 * chart_regularity_weight;
            emergency_neighbour_weight = 4 * chart_regularity_weight;
        } else if (valence == 4) {
            emergency_side_loop_weight = 4 * chart_regularity_weight;
            emergency_neighbour_weight = 4 * chart_regularity_weight;
        } else if (valence == 5) {
            emergency_side_loop_weight = 4 * chart_regularity_weight;
            emergency_neighbour_weight = 2 * chart_regularity_weight;
        } else if (valence == 6) {
            emergency_side_loop_weight = 1 * chart_regularity_weight;
            emergency_neighbour_weight  = 3 * chart_regularity_weight;
            singularity_on_boundary_weight = .5 * chart_regularity_weight;
        }


        auto connect_chart_sides = [&](Node a, Node b, BiMDF::TargetScalar estimate, size_t valence) {
            assert (a != lemon::INVALID);
            assert (b != lemon::INVALID);

            bimdf.add_edge({.u=a, .v=b, .u_head=false, .v_head=false,
                            .cost_function = Satsuma::CostFunction::Zero{.guess=estimate},
                            .lower=(valence == 4 ? 0 : 1),
                            .upper=bimdf.inf()});
            if (valence != 4 // no costs here in ILP formulation
    #if 0
                    && valence !=6 // ILP formulation only allows parity >= 6, this approximates it, but is too limiting
    #endif
                    ){
                auto e = bimdf.add_edge({
                                            .u=a, .v=b, .u_head=true, .v_head=true,
                                            .cost_function = Satsuma::CostFunction::AbsDeviation{
                                                .target=0, .weight=singularity_on_boundary_weight},
                                            .lower=0, .upper=1});
                problem.sing_on_bound_edges_per_valence[valence].push_back(e);

            }
        };


        // valence 3:
        //   emergency self-loops sufficient
        //   1 unit of self-loop emergency adds up to 2 to ilp energy (depends on flow between other edges)
        //   if there is more flow between other edges than strictly necessary, it's cheaper not to use emergency,
        //   but increase flow to either edge by 1. I *think* therefore 1 flow unit is always 2 ilp units
        //   with emergency node 1:1
        // valence 4:
        //   self-loops + diagonal emergency edges sufficient (4+4, better 1+4...)
        //   1 unit of diagonal adds 4 times one to ilp energy (for each edge)
        //   1 unit of self-loop adds 2 two times (side + its opposite)
        //   withe emergency node: 1:2
        // valence 5:
        //   emergency edges between neighbours sufficient -> 10 edges, better 1+5 with emergency node
        //   *can* be detected on either neighbor => up to 2 ilp energy for each flow unit
        // valence 6:
        //   emergency edges between opposite sides + neighbours -> 18 edges, better 1+6 with emergency node
        //   might make sense anyways for cost control, opposite vs neighbours
        //   in even/odd edge triplets: same as triangle, each unit adds (up to) 2
        //


        auto add_emergency_tailtail = [&](Node left, Node right, double weight) -> Edge {
            return bimdf.add_edge({.u = left, .v=right,
                                   .u_head = false, .v_head = false,
                                   .cost_function = Satsuma::CostFunction::AbsDeviation{.target=0, .weight=weight},
                                   .lower = 0, .upper = bimdf.inf()});
        };

        auto add_emergency_side_loop = [&](Node node, double weight) {
            auto emerg = add_emergency_tailtail(node, node, weight);
            problem.emergency_sideloops_per_valence[valence].push_back(emerg);
        };

        std::vector<Node> &side_nodes = chart_side_nodes[chart_id];
        side_nodes.resize(valence, lemon::INVALID);

        std::vector<std::array<Node, 2>> &side_node_pairs = chart_side_node_pairs[chart_id]; // 2 per paired side
        side_node_pairs.resize(valence, {lemon::INVALID, lemon::INVALID});


        for (size_t side_idx = 0; side_idx < valence; side_idx++)
        {
            if (side_paired[side_idx]) {
                for (size_t i = 0; i < 2; ++i) {
                  auto n = bimdf.add_node();
                  side_node_pairs[side_idx][i] = n;
                  add_emergency_side_loop(n, emergency_side_loop_weight);
                }
#if 1
                auto e = add_emergency_tailtail(
                        side_node_pairs[side_idx][0],
                        side_node_pairs[side_idx][1],
                        emergency_side_loop_weight);
                problem.emergency_sideloops_per_valence[valence].push_back(e);
#endif
            } else {
                auto side_node = bimdf.add_node();
                side_nodes[side_idx] = side_node;
                add_emergency_side_loop(side_node, emergency_side_loop_weight);
            }
        }

        auto add_emergency_edge = [&](int left, int right, double weight, std::vector<Edge> &out_edge_vec) {
            auto connect = [&] (Node a, Node b) {
                out_edge_vec.push_back(add_emergency_tailtail(a, b, weight));
            };
            if (side_paired[left] && !side_paired[right]) {
                // eliminate one case below
                std::swap(left, right);
            }
            if (side_paired[left]) {
                if (side_paired[right]) {
                    // both paired:
                    connect(side_node_pairs[left][0], side_node_pairs[right][0]);
                    connect(side_node_pairs[left][1], side_node_pairs[right][0]);
                    connect(side_node_pairs[left][0], side_node_pairs[right][1]);
                    connect(side_node_pairs[left][1], side_node_pairs[right][1]);
                } else {
                    throw std::runtime_error("implementation error, should have been swapped");
                }
            } else {
                if (side_paired[right]) {
                    // one paired:
                    connect(side_nodes[left], side_node_pairs[right][0]);
                    connect(side_nodes[left], side_node_pairs[right][1]);
                } else {
                    connect(side_nodes[left], side_nodes[right]);
                }
            }
        };
        if (valence > 3) {
          // connect direct neighbors with emergency loops
          for (int side_idx = 0; side_idx < valence; ++side_idx) {
            add_emergency_edge(
                    side_idx,
                    (side_idx + 1) % valence,
                    emergency_neighbour_weight,
                    problem.emergency_neighbor_per_valence[valence]);
          }
        }
        if (valence == 6) {
          // an unit of flow for opposite sides changes parity
          // opposite sides
          auto w = 6 * chart_regularity_weight;
          add_emergency_edge(0, 3, w, problem.emergency_opposite_v6);
          add_emergency_edge(1, 4, w, problem.emergency_opposite_v6);
          add_emergency_edge(2, 5, w, problem.emergency_opposite_v6);
        }

        // connect side nodes to their partners that allow regular quantisation

        // a valance-4 patch will visit the same side/other pairs twice if we loop until "valence",
        // avoid that:
        const size_t n_inner_edges = valence == 4 ? 2: valence;
        for (size_t side_idx = 0; side_idx < n_inner_edges; side_idx++) {
            const ChartSide& this_side = chart.chartSides.at(side_idx);
            auto this_target= this_side.length / chart_edge_length[chart_id];
            size_t other_idx = (side_idx + 2) % valence;
            const ChartSide& other_side = chart.chartSides.at(other_idx);
            auto other_target = other_side.length / chart_edge_length[chart_id];
            // estimate likely throughflow to set a decent target for initial rounding
            auto estimate = .25 * (this_target + other_target);

            if (side_paired[side_idx]) {
                if (side_paired[other_idx]) {
                    // both paired:
                    connect_chart_sides(side_node_pairs[side_idx][1], side_node_pairs[other_idx][0], estimate, valence);
                    if (valence == 4) { // because we only loop 2 times, not 4 - connect both!
                        connect_chart_sides(side_node_pairs[side_idx][0], side_node_pairs[other_idx][1], estimate, valence);
                    }
                } else {
                    assert(valence != 4);
                    // side paired, other regular
                    connect_chart_sides(side_node_pairs[side_idx][1], side_nodes[other_idx], estimate, valence);
                }
            } else {
                if (side_paired[other_idx]) {
                    // side regular, other paired
                    assert(valence != 4);
                    // use other-0, as the iteration where the current other_idx is side_idx will connect other-1:
                    connect_chart_sides(side_node_pairs[other_idx][0], side_nodes[side_idx], estimate, valence);
                } else {
                    // TODO: is this edge is added twice?
                    // no pairing:
                    connect_chart_sides(side_nodes[side_idx], side_nodes[other_idx], estimate, valence);
                }
            }
        }
    }
    //std::cout << "flow: " << n_unused_charts << " unused." << std::endl;


    Node boundary = bimdf.add_node();
    double bnd_target = 0;

    for (size_t subside_id = 0; subside_id < chart_data.subsides.size(); subside_id++)
    {
        const ChartSubside& subside = chart_data.subsides.at(subside_id);

        // arbitrarily call the sides "left" and "right":
        int left_chart_id = subside.incidentCharts[0];
        int right_chart_id = subside.incidentCharts[1];
        int left_side_idx = subside.incidentChartSideId[0];
        int right_side_idx = subside.incidentChartSideId[1];

        if (!should_use_chart(left_chart_id) || !should_use_chart(right_chart_id)) {
            // if this really happens, we should probably constrain all its subsides to 0?
            std::cerr << "WARNING: should not use chart!" << std::endl;
            continue;
        }

        // if one side is boundary, make sure it is the 'right' one:
        if (left_chart_id == -1) {
            std::swap(left_chart_id, right_chart_id);
            std::swap(left_side_idx, right_side_idx);
        }

        // determine average subside target length:
        double left_target = subside.length / chart_edge_length[left_chart_id];
        double right_target = left_target;
        const auto &left_chart = chart_data.charts[left_chart_id];
        const auto &right_chart = chart_data.charts[right_chart_id];

        int left_n_subsides = left_chart.chartSubsides.size();
        int right_n_subsides = left_n_subsides;
        if (right_chart_id != -1) {
            right_n_subsides = right_chart.chartSubsides.size();
            right_target = subside.length / chart_edge_length[right_chart_id];
        }
        left_target = std::max(1., left_target); // TODO: just as in ILP - but probably makes results worse
        right_target = std::max(1., right_target);

        assert(left_side_idx != -1);
        bool left_paired = spi.paired_sides[left_chart_id][left_side_idx];
        auto const& left_pair = chart_side_node_pairs[left_chart_id][left_side_idx];
        auto const& left_single = chart_side_nodes.at(left_chart_id).at(left_side_idx);


        double left_iso_weight = isometry_weight / left_n_subsides;
        double right_iso_weight = isometry_weight / right_n_subsides;
        double avg_valence = .5 * (left_n_subsides + right_n_subsides);
        double unaligned_cost = regularity_weight * parameters.alignSingularitiesWeight * .5 / avg_valence;

        auto &edges = problem.subside_edges[subside_id];
        if (right_chart_id == -1) {
            if (left_paired) {
                assert(false); // this should never happen
#if 0
                edges[0] = add_subside_edge(left_pair[0], true, boundary, true, .5 * target, paired_subside_weight);
                edges[1] = add_subside_edge(left_pair[1], true, boundary, true, .5 * target, paired_subside_weight);
#endif
            } else {
                edges[0] = add_subside_edge(left_single, true, boundary, true, left_target, left_iso_weight);
                bnd_target += left_target;
                problem.unpaired_edges.push_back(edges[0]);
            }
        } else {
            // no boundary involved!
            bool right_paired = spi.paired_sides[right_chart_id][right_side_idx];
            // these may be invalid depening on paired/non-paired:
            auto const& right_pair = chart_side_node_pairs[right_chart_id][right_side_idx];
            auto const& right_single = chart_side_nodes.at(right_chart_id).at(right_side_idx);


            if (left_paired && right_paired) {
#if 1  // check for oriented surface
            const auto &left_side = left_chart.chartSides[left_side_idx];
            const auto &right_side = right_chart.chartSides[right_side_idx];
            assert(left_side.vertices.front() == right_side.vertices.back());
            assert(left_side.vertices.back() == right_side.vertices.front());
#endif
                assert(left_side.subsides.size() == 1);
                assert(right_side.subsides.size() == 1);
                std::array<Node, 2> inter = {bimdf.add_node(), bimdf.add_node()};
                TargetsAndWeight left_taw, right_taw;

                if (flow_config.paired_half_target == PairedHalfTarget::Half) {
                    left_taw.targets = {.5 * left_target, .5 * left_target};
                    right_taw.targets = {.5 * right_target, .5 * right_target};
                    left_taw.weight = left_chart.chartSides.size() == 4 ? 0.: 1.;
                    right_taw.weight = right_chart.chartSides.size() == 4 ? 0.: 1.;
                } else if (flow_config.paired_half_target == PairedHalfTarget::Simple) {
                    left_taw = virtual_subside_target(chart_data, chart_edge_length, left_chart_id, left_side_idx);
                    right_taw = virtual_subside_target(chart_data, chart_edge_length, right_chart_id, right_side_idx);
                } else {
                    throw std::runtime_error("unknown paired_half_target");
                }

                auto iso_obj             = flow_config.paired_initial.iso_objective;
                double aligned_iso_scale = flow_config.paired_initial.iso_weight;
                double unalign_weight    = flow_config.paired_initial.unalign_weight;

                if (previous_problem && previous_sol) {
                    iso_obj           = flow_config.paired_resolve.iso_objective;
                    aligned_iso_scale = flow_config.paired_resolve.iso_weight;
                    unalign_weight    = flow_config.paired_resolve.unalign_weight;
                }

                if (flow_config.paired_resolve_new_targets && previous_problem && previous_sol) {
                    auto const &sol = *previous_sol;
                    auto const &old_edges = previous_problem->subside_edges[subside_id];
                    auto sol0 = sol[old_edges[0]];
                    auto sol1 = sol[old_edges[1]];
                    auto solsum = sol0 + sol1;
                    auto left_scale = left_target / solsum;
                    left_taw.targets[0] = left_scale * sol0;
                    left_taw.targets[1] = left_target - left_taw.targets[0];
                    // TODO TODO TODO right target with proper old results (or always same, if aligned (and thus not dropped)?)
                    auto right_scale = right_target / solsum;
                    right_taw.targets[1] = right_scale * sol0;
                    right_taw.targets[0] = right_target - right_taw.targets[1];

#if 0
                    left_taw.targets[0] = sol[old_edges[0]];
                    left_taw.targets[1] = sol[old_edges[1]];
#endif
#if 0
                    std::cout << "   PAIR target " << left_target
                              << ", old sols: " << sol0 << ", " << sol1
                              << ", new tgt: " << left_target_0 << ", " << left_target_1
                              << std::endl;
#endif
                }

                // lower bounds for virtual subside connected via inter[i]:
                // TODO: choose which one may be zero based on target lengths and weights
                int lower0 = 1;
                int lower1 = 1;
                edges[0] = add_subside_edge(inter[0], true,   left_pair[0], true,
                        left_taw.targets[0], aligned_iso_scale * left_taw.weight * left_iso_weight, iso_obj, lower0);
                edges[1] = add_subside_edge(inter[1], true,   left_pair[1], true,
                        left_taw.targets[1], aligned_iso_scale * left_taw.weight * left_iso_weight, iso_obj, lower1);

                // swaped array indices intentional to connect corresponding pair nodes:
                auto other0 = add_subside_edge(inter[0], false, right_pair[1], true,
                        right_taw.targets[1], aligned_iso_scale * right_taw.weight * right_iso_weight, iso_obj, lower0);
                auto other1 = add_subside_edge(inter[1], false, right_pair[0], true,
                        right_taw.targets[0], aligned_iso_scale * right_taw.weight * right_iso_weight, iso_obj, lower1);
                problem.paired_edges.push_back(edges[0]);
                problem.paired_edges.push_back(edges[1]);
                problem.paired_edges.push_back(other0);
                problem.paired_edges.push_back(other1);

                // allow non-alignment:
#if 1 // disabling is just for debug
                auto e1  = bimdf.add_edge({ .u = inter[0], .v = inter[1],
                                            .u_head = false, .v_head = true,
                                            .cost_function = Satsuma::CostFunction::AbsDeviation{
                                                .target = 0, .weight = unaligned_cost * unalign_weight}});
                auto e2  = bimdf.add_edge({ .u = inter[1], .v = inter[0],
                                            .u_head = false, .v_head = true,
                                            .cost_function = Satsuma::CostFunction::AbsDeviation{
                                                .target = 0, .weight = unaligned_cost * unalign_weight}});
                problem.pair_unaligners.push_back(e1);
                problem.pair_unaligners.push_back(e2);
#endif

            } else if (!left_paired && !right_paired) {
                Node inter = bimdf.add_node(); // only used to model sum of quadratic functions
                edges[0] = add_subside_edge(left_single, true, inter, true, left_target, left_iso_weight);
                auto other_edge = add_subside_edge(inter, false, right_single, true, right_target, right_iso_weight);
                problem.unpaired_edges.push_back(edges[0]);
                problem.unpaired_edges.push_back(other_edge);
            } else if (!left_paired && right_paired) {
                assert(false); // i think this case should not happen.
            } else if ( left_paired && !right_paired) {
                assert(false); // i think this case should not happen.
            }
        }
    }
    [[maybe_unused]] auto bnd_loop = bimdf.add_edge({ .u = boundary,
                     .v = boundary,
                     .u_head = false,
                     .v_head = false,
                     .cost_function = Satsuma::CostFunction::Zero{.guess=bnd_target/2}
                   });
    std::cout << "\tbimdf problem: "
              << g.maxNodeId() + 1 << " nodes, "
              << g.maxArcId() + 1 << " arcs.\n";

    return problem;

}


bool is_valid_quantisation(
        const ChartData& chart_data,
        const std::vector<int>& lens)
{
    bool valid = true;
    for (size_t chart_id = 0; chart_id < chart_data.charts.size(); ++chart_id)
    {
        const Chart& chart = chart_data.charts.at(chart_id);
        size_t boundary_sum = 0;
        for (size_t side_idx = 0; side_idx < chart.chartSides.size(); side_idx++) {
            const ChartSide& side = chart.chartSides.at(side_idx);
            int side_sum = 0;

            for (const auto subside_id: side.subsides) {
                side_sum += lens.at(subside_id);
            }
            if (side_sum <= 0 || side_sum >= (1<<20)) {
                std::cerr << "flow quantisation error: illegal side sum in chart "
                            << chart_id
                            << ", side " << side_idx
                            << ": " << side_sum
                            << std::endl;
                valid = false;
            }
            boundary_sum += side_sum;
        }
        if (boundary_sum & 1 || boundary_sum < 4) {
            std::cerr << "flow quantisation error: illegal boundary sum in chart "
                      << chart_id
                      << ": " << boundary_sum
                      << std::endl;
            valid = false;
        }
    }
    return valid;
}



#define ILP_FIND_SUBDIVISION -1
#define ILP_IGNORE -2

template<typename Config>
static Config get_json_config(std::string filename) {
    Config config;
    std::string name = typeid(Config).name();

    if (!filename.empty()) {
        std::ifstream f(filename);
        if (!f.good()) {
            throw std::runtime_error("Could not open " + name + " config file '"
                    + filename + "'");
        }
        nlohmann::json j;
        f >> j;
        try {
            config = j;
        } catch (nlohmann::json::exception const &e) {
            std::cerr << "json contents of " << filename << " incompatible with " << name
                << ", contents: \n" << std::setw(4) << j << std::endl;
            throw;
        }
        std::cout << "using " + name + " config from '"
                  << filename
                  << '"' << std::endl;
    } else {
        std::cout << "using default " + name + "config:" << std::endl;
    }
    std::cout << std::setw(4) << nlohmann::json(config) << std::endl;
    return config;
}

FlowResult findSubdivisionsFlow(
        const ChartData& chart_data,
        const std::vector<double>& chart_edge_length,
        const Parameters& parameters,
        double& out_gap,
        std::vector<int>& out_results)
{
    using HSW = Timekeeper::HierarchicalStopWatch;
    HSW sw_root{"find_subdivisions_flow"};
    HSW sw_singularity_pairs{"find_singularity_pairs", sw_root};
    HSW sw_setup{"setup", sw_root};
    HSW sw_analysis{"analysis", sw_root};
    sw_root.resume();

    auto flow_config = get_json_config<FlowConfig>(parameters.flow_config_filename);
    auto satsuma_config = get_json_config<Satsuma::BiMDFSolverConfig>(parameters.satsuma_config_filename);

    std::vector<FlowStats> stats;

    assert(out_results.size() == chart_data.subsides.size());
    for (size_t subside_id = 0; subside_id < out_results.size(); ++subside_id)
    {
        auto val = out_results[subside_id];
        assert (val == ILP_FIND_SUBDIVISION); // we don't handle anything else yet
    }

    std::vector<bool> satisfied_regularity;
    std::vector<bool> satisfied_alignment;
    sw_singularity_pairs.resume();
    SingularityPairInfo spi = find_singularity_pairs(chart_data);
    sw_singularity_pairs.stop();
    if (!parameters.alignSingularities) {
        std::fill(spi.paired_sides.begin(),
                  spi.paired_sides.end(),
                PairedSides{false,false,false,false,false, false});
        spi.pairs.clear();
    }



    std::vector<Satsuma::BiMDFFullResult> bimdf_results;
    auto solve_and_apply = [&](FlowProblem const &problem) {

        std::cout << "\nflow problem setup complete, solving..." << std::endl;
        auto res = solve_bimdf(*problem.bimdf, satsuma_config);

        const auto &sol = *res.solution.get();
        bimdf_results.push_back(std::move(res));

        Timekeeper::ScopedStopWatch _{sw_analysis};
        if (!problem.bimdf->is_valid(sol)) {
            throw std::runtime_error("Internal error: BiMDF solution invalid");
        }
        double bimdf_cost = problem.bimdf->cost(sol);
        std::cout << "flow solved. cost:  " << bimdf_cost << std::endl;

        assert(out_results.size() == chart_data.subsides.size());
        for (size_t i = 0; i < out_results.size(); ++i) {
            const auto &edges = problem.subside_edges[i];

            assert (edges[0] != lemon::INVALID);
            auto val = sol[edges[0]];

            if (edges[1] != lemon::INVALID) {
                val += sol[edges[1]];
            }
            if (val < 1) {
                std::cout << "ERROR: subside " << i << " quantized to " << val << std::endl;
            }
            out_results[i] = val;
        }
        auto count_uses = [&](const auto &edge_vec) -> size_t {
            size_t n = 0;
            for(const auto &e: edge_vec) {
                if (sol[e] != 0) {
                    ++n;
                }
            }
            return n;
        };
        auto sum_costs = [&](const auto &edge_vec) -> double {
            double sum = 0;
            for(const auto &e: edge_vec) {
                sum += problem.bimdf->cost(e, sol[e]);
            }
            return sum;
        };
#if 0
        auto debug_costs = [&](const auto &edge_vec) {
            for(auto e: edge_vec) {
                if (sol[e] != 0) {
                    std::cout << "edge " << problem.bimdf->g.id(e)
                            << ": " << sol[e]
                            << ", cost " << problem.bimdf->cost(e, sol[e])
                            << std::endl;
                }
            }
        };
#endif
#ifndef _MSC_VER // XXX TODO: this is a weird MSVC compile error, just ignore stats
        //debug_costs(problem.emergency_sideloops_per_valence[5]);
        stats.push_back(FlowStats{
                            .n_pair_unaligners_used = count_uses(problem.pair_unaligners),
                            .cost_iso_nonpaired = sum_costs(problem.unpaired_edges),
                            .cost_iso_paired = sum_costs(problem.paired_edges),
                            .cost_regularity_neighbor_v3 = sum_costs(problem.emergency_neighbor_per_valence[3]),
                            .cost_regularity_neighbor_v4 = sum_costs(problem.emergency_neighbor_per_valence[4]),
                            .cost_regularity_neighbor_v5 = sum_costs(problem.emergency_neighbor_per_valence[5]),
                            .cost_regularity_neighbor_v6 = sum_costs(problem.emergency_neighbor_per_valence[6]),
                            .cost_regularity_opposite_v6 = sum_costs(problem.emergency_opposite_v6),
                            .cost_regularity_sideloops_v3 = sum_costs(problem.emergency_sideloops_per_valence[3]),
                            .cost_regularity_sideloops_v4 = sum_costs(problem.emergency_sideloops_per_valence[4]),
                            .cost_regularity_sideloops_v5 = sum_costs(problem.emergency_sideloops_per_valence[5]),
                            .cost_regularity_sideloops_v6 = sum_costs(problem.emergency_sideloops_per_valence[6]),
                            .cost_sing_on_bound_v3 = sum_costs(problem.sing_on_bound_edges_per_valence[3]),
                            .cost_sing_on_bound_v5 = sum_costs(problem.sing_on_bound_edges_per_valence[5]),
                            .cost_sing_on_bound_v6 = sum_costs(problem.sing_on_bound_edges_per_valence[6]),
                            .cost_alignment = sum_costs(problem.pair_unaligners),
                        });


        FlowStats &stat = stats.back();

        std::cout << "\nsolved. FlowStats:"
                  << std::setw(4)
                  << nlohmann::json(stat)
                  << "\n\tsummed: " << stat.cost_sum() // TODO: assert to eps-equality with total cost
                  << "\n" << std::endl;
        double err = std::fabs(stat.cost_sum() - bimdf_cost);
        if (std::fabs(err) > 1e-3) {
            std::cerr << "ERROR: BiMDF cost ("
                      << bimdf_cost
                      << ") does not match our edge tracking ("
                      << stat.cost_sum()
                      << ")." << std::endl;
        }
#endif

        if (is_valid_quantisation(chart_data, out_results)) {
            //std::cout << "\tvalidation of hard constraints successful." << std::endl;
        } else {
            throw std::runtime_error("flow quantisation resulted in infeasible result.");
        }
    };


    /// return true iff we have dropped constraints
    auto update_satisfaction = [&]() -> bool
    {
        Timekeeper::ScopedStopWatch _{sw_analysis};

        if (!parameters.repeatLosingConstraintsAlign
                && !parameters.repeatLosingConstraintsQuads
                && !parameters.repeatLosingConstraintsNonQuads)
        {
            // No need to update if we never re-solve.
            return false;
        }
        satisfied_regularity.resize(chart_data.charts.size());

        size_t n_unsat_reg = 0;
        for (size_t chart_id = 0; chart_id < chart_data.charts.size(); ++chart_id)
        {
            const size_t valence = chart_data.charts[chart_id].chartSides.size();
            // NB: 1 is still okay, it means the singlarity is on the boundary
            int max_ok = valence == 4 ? 0 : 1;
            bool sat = max_ok >= chart_irregularity(chart_data, out_results, chart_id);
            satisfied_regularity[chart_id] = sat;
            if (!sat) {
                ++n_unsat_reg;
            }
        }
        std::cout << n_unsat_reg
                  << " unsatisfied regularity constraints."
                  << std::endl;

        satisfied_alignment.resize(spi.pairs.size());

        size_t n_unsat_align = 0;
        for (size_t pair_id = 0; pair_id < spi.pairs.size(); ++pair_id)
        {
            const auto &pair = spi.pairs.at(pair_id);
            bool all_regular = true;
            for (int i = 0; i < 2; ++i) {
                if (!satisfied_regularity[pair.charts[i]]) {
                    all_regular = false;
                    break;
                }
            }
            if (all_regular) {
                for (const auto &quad: pair.quads) {
                    if (!satisfied_regularity[quad.chart]) {
                        all_regular = false;
                        break;
                    }
                }
            }
            // TODO: do we want to remove alignment pairs that are satisfied
            //       althought the corresponding flow values do not ensure it?
            bool sat = all_regular &&  is_singularity_pair_aligned(chart_data, out_results, pair);

            satisfied_alignment[pair_id] = sat;
            if(!sat) {
                ++n_unsat_align;
            }
        }
        if (n_unsat_align == 0 && n_unsat_reg == 0) {
            return false;
        }
        if (parameters.repeatLosingConstraintsAlign) {
            std::cout << "removing "
                      << n_unsat_align
                      << " unsatisfied alignment constraints."
                      << std::endl;

            spi.remove_unaligned_pairs(satisfied_alignment);
            return true;
        } else {
            return false;
        }
    };


    sw_setup.resume();

    FlowProblem problem = make_bimdf(
            flow_config,
            chart_data,
            chart_edge_length,
            parameters,
            spi,
            satisfied_regularity);
    sw_setup.stop();

    solve_and_apply(problem);

    sw_analysis.resume();
    std::cout << "\nflow round one finished. stats:\n"
              << evaluate_quantization(chart_data, chart_edge_length, parameters, out_results)
              << std::endl;
    sw_analysis.stop();
    bool updated = update_satisfaction();
    if (updated) {
        // TODO PERF: pass old solution as x0 (need to handle dropped alignment constraints!)
        sw_setup.resume();
        auto new_problem = make_bimdf(
                flow_config,
                chart_data,
                chart_edge_length,
                parameters,
                spi,
                satisfied_regularity,
                &problem,
                bimdf_results.back().solution.get());

        sw_setup.stop();
        solve_and_apply(new_problem);
    }

    out_gap = 0;
    sw_root.stop();
    auto sw_result = Timekeeper::HierarchicalStopWatchResult(sw_root);
    HSW sw_solve{"solve"};
    auto sw_solve_result = Timekeeper::HierarchicalStopWatchResult(sw_solve);
    for (size_t i = 0; i < bimdf_results.size(); ++i) {
        auto sw = bimdf_results[i].stopwatch;
        sw.name = std::to_string(i);
        sw_solve_result.add_child(std::move(sw));
    }
    sw_result.add_child(std::move(sw_solve_result));

    return {.bimdf_results = std::move(bimdf_results),
            .stats = std::move(stats),
            .stopwatch = std::move(sw_result)};
}

} // namespace QuadRetopology
