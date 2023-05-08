/***************************************************************************/
/* Copyright(C) 2021


The authors of

Reliable Feature-Line Driven Quad-Remeshing
Siggraph 2021


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

#include "quad_from_patches.h"
#include <quadretopology/quadretopology.h>
#include <quadretopology/qr_eval_quantization.h>
#include <random>

#ifdef SAVE_MESHES_FOR_DEBUG
#include <igl/writeOBJ.h>
#include <wrap/io_trimesh/export_obj.h>
#endif


namespace qfp {

template<class PolyMesh, class TriangleMesh>
QuadrangulationResult
quadrangulationFromPatches(
    TriangleMesh& trimesh,
    const std::vector<std::vector<size_t>>& trimeshPartitions,
    const std::vector<std::vector<size_t>>& trimeshCorners,
    const std::vector<double>& chartEdgeLength,
    const QuadRetopology::Parameters& parameters,
    const int fixedChartClusters,
    PolyMesh& quadmesh,
    std::vector<std::vector<size_t>>& quadmeshPartitions,
    std::vector<std::vector<size_t>>& quadmeshCorners,
    std::vector<int>& ilpResult)
{
    using HSW = Timekeeper::HierarchicalStopWatch;
    HSW sw_root{"qfp"};
    HSW sw_quadrangulate("quadrangulate", sw_root);
    HSW sw_compute_chart_data("compute_chart_data", sw_root);

    std::vector<Satsuma::BiMDFFullResult> bimdf_results; // empty if ILP was used
    std::vector<QuadRetopology::FlowStats> flow_stats;
    std::vector<std::vector<QuadRetopology::ILPStats>> ilp_stats_per_cluster;

    sw_root.resume();
    std::vector<Timekeeper::HierarchicalStopWatchResult> sw_results;

    assert(trimeshPartitions.size() == trimeshCorners.size() && chartEdgeLength.size() == trimeshPartitions.size());


    //Get chart data
    sw_compute_chart_data.resume();
    QuadRetopology::ChartData chartData = QuadRetopology::computeChartData(
            trimesh,
            trimeshPartitions,
            trimeshCorners);
    sw_compute_chart_data.stop();


    //Initialize ilp results
    ilpResult.clear();
    ilpResult.resize(chartData.subsides.size(), ILP_FIND_SUBDIVISION);


    bool solvedCluster = false;
    if (fixedChartClusters > 0 && fixedChartClusters < static_cast<int>(chartData.charts.size())) {
        std::vector<int> chartCluster(chartData.charts.size(), -1);
        bool clusterDone;
        int lastClusterId = 0;
        do {
            clusterDone = false;
            size_t firstChart = std::numeric_limits<size_t>::max();
            for (size_t cId = 0; cId < chartData.charts.size() && firstChart == std::numeric_limits<size_t>::max(); ++cId) {
                if (chartCluster[cId] == -1) {
                    firstChart = cId;
                }
            }
            if (firstChart < std::numeric_limits<size_t>::max()) {
                int numInCluster = 0;

                std::queue<size_t> queue;
                queue.push(firstChart);

                while (!queue.empty() && numInCluster < fixedChartClusters) {
                    size_t currentChart = queue.front();
                    queue.pop();

                    if (chartCluster[currentChart] > -1)
                        continue;

                    const QuadRetopology::Chart& chart = chartData.charts[currentChart];
                    chartCluster[currentChart] = lastClusterId;
                    numInCluster++;

                    for (size_t adjCId : chart.adjacentCharts) {
                        if (chartCluster[adjCId] == -1) {
                            if (chartCluster[adjCId] == -1)
                                queue.push(adjCId);
                        }
                    }
                }

                std::cout << "Initial cluster " << lastClusterId << " composed of " << numInCluster << " charts." << std::endl;

                lastClusterId++;
            }
            else {
                clusterDone = true;
            }

        } while (!clusterDone);

        const int limitChartsForCluster = std::max(fixedChartClusters / 3, std::min(fixedChartClusters, 10));

        for (int i = 0; i < lastClusterId; ++i) {
            int numInCluster = 0;

            std::set<size_t> chartsInCluster;
            for (size_t cId = 0; cId < chartData.charts.size(); ++cId) {
                if (chartCluster[cId] == i) {
                    numInCluster++;
                    chartsInCluster.insert(cId);
                }
            }

            if (numInCluster < limitChartsForCluster) {
                bool found = true;
                while (!chartsInCluster.empty() && found) {
                    size_t targetChart = std::numeric_limits<size_t>::max();
                    size_t targetCluster = std::numeric_limits<size_t>::max();

                    std::set<size_t>::iterator it = chartsInCluster.begin();

                    do {
                        size_t cId = *it;
                        const QuadRetopology::Chart& chart = chartData.charts[cId];
                        for (size_t adjCId : chart.adjacentCharts) {
                            if (chartCluster[cId] == i && chartCluster[adjCId] != -1 && chartCluster[adjCId] != i) {
                                targetChart = cId;
                                targetCluster = chartCluster[adjCId];
                                break;
                            }
                        }
                        if (targetChart == std::numeric_limits<size_t>::max()) {
                            it++;
                        }
                    } while (it != chartsInCluster.end() && targetChart == std::numeric_limits<size_t>::max());

                    found = false;

                    if (targetChart != std::numeric_limits<size_t>::max()) {
                        chartCluster[targetChart] = targetCluster;
                        chartsInCluster.erase(targetChart);

                        found = true;
                    }
                }
            }
        }

        int numClusters = 0;
        for (int clusterId = 0; clusterId < lastClusterId; ++clusterId) {
            int numInCluster = 0;

            for (size_t cId = 0; cId < chartData.charts.size(); ++cId) {
                if (chartCluster[cId] == clusterId) {
                    numInCluster++;
                }
            }

            if (numInCluster > 0) {
                numClusters++;

                std::cout << "Cluster " << clusterId << " is composed of " << numInCluster << " charts." << std::endl;
            }
        }

        if (numClusters > 1) {
            std::cout << "Patches clustered in: " << numClusters << " clusters." << std::endl;

            int numFixed = 0;

            std::vector<bool> subsideAlreadyFixed(chartData.subsides.size(), false);
            for (size_t subsideId = 0; subsideId < chartData.subsides.size(); subsideId++) {
                std::array<int, 2> incidentCharts = chartData.subsides[subsideId].incidentCharts;
                const QuadRetopology::ChartSubside& subside = chartData.subsides[subsideId];

                if (
                        incidentCharts[0] != -1 &&
                        incidentCharts[1] != -1 &&
                        chartCluster[incidentCharts[0]] != chartCluster[incidentCharts[1]]
                )
                {
                    if (!subsideAlreadyFixed[subsideId])
                    {
                        double edgeLength = 0.0;
                        int numIncident = 0;
                        for (int cId : incidentCharts) {
                            if (cId >= 0) {
                                edgeLength += chartEdgeLength[cId];
                                numIncident++;
                            }
                        }
                        edgeLength /= numIncident;

                        double sideSubdivision = subside.length / edgeLength;
                        if (!parameters.hardParityConstraint) {
                            sideSubdivision /= 2.0;
                        }

                        //Round to nearest even number
                        sideSubdivision = std::round(sideSubdivision / 2.0) * 2.0;

                        sideSubdivision = std::max(static_cast<double>(MIN_SUBDIVISION_VALUE*2), sideSubdivision);

                        ilpResult[subsideId] = static_cast<int>(std::round(sideSubdivision));
                        assert(ilpResult[subsideId] % 2 == 0);

                        numFixed++;

                        subsideAlreadyFixed[subsideId] = true;
                    }
                }
            }

            std::cout << "Fixed " << numFixed << " / " << chartData.subsides.size() << " subsides." << std::endl;



            for (int clusterId = 0; clusterId < lastClusterId; ++clusterId) {
                int numInCluster = 0;

                std::vector<int> result(chartData.subsides.size(), ILP_IGNORE);
                for (size_t subsideId = 0; subsideId < chartData.subsides.size(); subsideId++) {
                    if (ilpResult[subsideId] >= 0) {
                        result[subsideId] = ilpResult[subsideId];
                    }
                }

                for (size_t cId = 0; cId < chartData.charts.size(); ++cId) {
                    if (chartCluster[cId] == clusterId) {
                        numInCluster++;
                    }
                }

                if (numInCluster > 0) {
                    solvedCluster = true;
                    for (size_t subsideId = 0; subsideId < chartData.subsides.size(); subsideId++) {
                        std::array<int, 2> incidentCharts = chartData.subsides[subsideId].incidentCharts;

                        if ((incidentCharts[0] == -1 || chartCluster[incidentCharts[0]] == clusterId) && (incidentCharts[1] == -1 || chartCluster[incidentCharts[1]] == clusterId)) {
                            result[subsideId] = ILP_FIND_SUBDIVISION;
                        }
                    }

                    double gap;
                    {
                      auto subdiv_res = QuadRetopology::findSubdivisions(
                          chartData,
                          chartEdgeLength,
                          parameters,
                          gap,
                          result);
                      sw_results.push_back(std::move(subdiv_res.stopwatch));
                      assert(subdiv_res.bimdf_results.empty()); // Flow does not work on clusters
                      if (!subdiv_res.ilp_stats.empty()) {
                          ilp_stats_per_cluster.push_back(std::move(subdiv_res.ilp_stats));
                      }
                    }

                    for (size_t subsideId = 0; subsideId < chartData.subsides.size(); subsideId++) {
                        if (result[subsideId] != ILP_IGNORE && ilpResult[subsideId] == ILP_FIND_SUBDIVISION) {
                            ilpResult[subsideId] = result[subsideId];
                        }
                    }
                }
            }
        }
    }

    if (!solvedCluster) {
        //Solve ILP to find best side size
        double gap;
        {
          auto subdiv_res = QuadRetopology::findSubdivisions(
              chartData,
              chartEdgeLength,
              parameters,
              gap,
              ilpResult);
          bimdf_results = std::move(subdiv_res.bimdf_results);
          flow_stats = std::move(subdiv_res.flow_stats);
          if (!subdiv_res.ilp_stats.empty()) {
              ilp_stats_per_cluster.push_back({std::move(subdiv_res.ilp_stats)});
          }

          sw_results.push_back(std::move(subdiv_res.stopwatch));
        }
    }

    auto quant_eval = QuadRetopology::evaluate_quantization(chartData, chartEdgeLength, parameters, ilpResult);
    std::cout << "quantisation evaluation results: \n " << quant_eval << std::endl;

    //Quadrangulate
    std::vector<size_t> fixedPositionSubsides;
    std::vector<int> quadmeshLabel;
    {
      Timekeeper::ScopedStopWatch _{sw_quadrangulate};
      QuadRetopology::quadrangulate(
              trimesh,
              chartData,
              fixedPositionSubsides,
              ilpResult,
              parameters,
              quadmesh,
              quadmeshLabel,
              quadmeshPartitions,
              quadmeshCorners);
    }
    sw_root.stop();
    auto sw_result = Timekeeper::HierarchicalStopWatchResult(sw_root);
    for (const auto &r: sw_results) {
        sw_result.add_child(r);
    }
    return {
        .bimdf_results = std::move(bimdf_results),
        .flow_stats = std::move(flow_stats),
        .ilp_stats_per_cluster = std::move(ilp_stats_per_cluster),
        .eval = std::move(quant_eval),
        .stopwatch = sw_result};
}

}
