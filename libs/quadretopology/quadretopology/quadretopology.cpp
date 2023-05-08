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

#include "quadretopology.h"

#include "includes/qr_convert.h"
#include "includes/qr_utils.h"
#include "includes/qr_patterns.h"
#include "includes/qr_mapping.h"
#include "qr_flow.h"
#include <map>

#include <vcg/complex/algorithms/polygonal_algorithms.h>

#ifdef QUADRETOPOLOGY_DEBUG_SAVE_MESHES
#include <igl/writeOBJ.h>
#endif

namespace QuadRetopology {


std::vector<double> computeChartEdgeLength(
        const ChartData& chartData,
        const size_t& iterations,
        const std::vector<int>& ilpResults,
        const double& weight)
{
    std::vector<double> avgLengths(chartData.charts.size() , -1);


    std::vector<bool> isFixed(chartData.subsides.size(), false);
    std::vector<bool> isComputable(chartData.subsides.size(), true);
    for (size_t subsideId = 0; subsideId < chartData.subsides.size(); ++subsideId) {
        if (ilpResults[subsideId] == ILP_IGNORE) {
            isComputable[subsideId] = false;
        }
        else if (ilpResults[subsideId] >= 0) {
            isFixed[subsideId] = true;
        }
    }

    //Fill charts with a border
    for (size_t i = 0; i < chartData.charts.size(); i++) {
        const Chart& chart = chartData.charts[i];
        if (chart.faces.size() > 0) {
            double currentQuadLength = 0;
            int numSides = 0;

            for (size_t sId : chart.chartSubsides) {
                const ChartSubside& subside = chartData.subsides[sId];
                if (isComputable[sId] && isFixed[sId]) {
                    currentQuadLength += subside.length / subside.size;
                    numSides++;
                }
            }

            if (numSides > 0) {
                currentQuadLength /= numSides;
                avgLengths[i] = currentQuadLength;
            }
        }
    }

    //Fill charts with no borders
    bool done;
    do {
        done = true;
        for (size_t i = 0; i < chartData.charts.size(); i++) {
            const Chart& chart = chartData.charts[i];
            if (chart.faces.size() > 0 && avgLengths[i] < 0) {
                double currentLength = 0;
                size_t numAdjacentCharts = 0;

                for (size_t adjId : chart.adjacentCharts) {
                    if (avgLengths[adjId] > 0) {
                        currentLength += avgLengths[adjId];
                        numAdjacentCharts++;
                        done = false;
                    }
                }

                if (currentLength > 0) {
                    currentLength /= numAdjacentCharts;
                    avgLengths[i] = currentLength;
                }
            }
        }
    } while (!done);


    //Smoothing
    for (size_t k = 0; k < iterations; k++) {
        std::vector<double> lastAvgLengths = avgLengths;

        for (size_t i = 0; i < chartData.charts.size(); i++) {
            const Chart& chart = chartData.charts[i];
            if (chart.faces.size() > 0) {
                assert(lastAvgLengths[i] > 0);

                double adjValue = 0.0;
                size_t numAdjacentCharts = 0;

                for (size_t adjId : chart.adjacentCharts) {
                    if (avgLengths[adjId] > 0) {
                        adjValue += lastAvgLengths[adjId];
                        numAdjacentCharts++;
                    }
                }

                if (adjValue > 0.0) {
                    adjValue = weight * lastAvgLengths[i] + (1.0 - weight) * adjValue;
                }
            }
        }
    }

    return avgLengths;
}


FindSubdivisionsResult findSubdivisions(
        const ChartData& chartData,
        const std::vector<double>& chartEdgeLength,
        const Parameters& parameters,
        double& gap,
        std::vector<int>& ilpResults)
{
    if (parameters.useFlowSolver) {
        auto res = findSubdivisionsFlow(
                chartData,
                chartEdgeLength,
                parameters,
                gap,
                ilpResults);
        return {.bimdf_results = std::move(res.bimdf_results),
                .flow_stats = std::move(res.stats),
                .stopwatch = std::move(res.stopwatch)};
    } else {
        Timekeeper::HierarchicalStopWatch sw{"ilp"};
        sw.resume();
        auto ilp_result = findSubdivisions(
            chartData,
            chartEdgeLength,
            parameters.ilpMethod,
            parameters.alpha,
            parameters.isometry,
            parameters.regularityQuadrilaterals,
            parameters.regularityNonQuadrilaterals,
            parameters.regularityNonQuadrilateralsWeight,
            parameters.alignSingularities,
            parameters.alignSingularitiesWeight,
            parameters.repeatLosingConstraintsIterations,
            parameters.repeatLosingConstraintsQuads,
            parameters.repeatLosingConstraintsNonQuads,
            parameters.repeatLosingConstraintsAlign,
            parameters.feasibilityFix,
            parameters.hardParityConstraint,
            parameters.timeLimit,
            parameters.gapLimit,
            parameters.callbackTimeLimit,
            parameters.callbackGapLimit,
            parameters.minimumGap,
            gap,
            ilpResults);
        sw.stop();
        return {
            .ilp_stats = ilp_result.stats,
            .stopwatch = sw};
    }
}

ILPResult findSubdivisions(
        const ChartData& chartData,
        const std::vector<double>& chartEdgeLength,
        const ILPMethod& method,
        const double alpha,
        const bool isometry,
        const bool regularityQuadrilaterals,
        const bool regularityNonQuadrilaterals,
        const double regularityNonQuadrilateralsWeight,
        const bool alignSingularities,
        const double alignSingularitiesWeight,
        const int repeatLosingConstraintsIterations,
        const bool repeatLosingConstraintsQuads,
        const bool repeatLosingConstraintsNonQuads,
        const bool repeatLosingConstraintsAlign,
        const bool feasibilityFix,
        const bool hardParityConstraint,
        const double timeLimit,
        const double gapLimit,
        const std::vector<float>& callbackTimeLimit,
        const std::vector<float>& callbackGapLimit,
        const double minimumGap,
        double& gap,
        std::vector<int>& ilpResults)
{
    if (chartData.charts.size() <= 0)
        return {};

    ILPStatus status;

    //Solve ILP to find the best patches
    std::vector<int> result = ilpResults;
    auto ilp_result = internal::solveILP(
        chartData,
        chartEdgeLength,
        method,
        alpha,
        isometry,
        regularityQuadrilaterals,
        regularityNonQuadrilaterals,
        regularityNonQuadrilateralsWeight,
        alignSingularities,
        alignSingularitiesWeight,
        repeatLosingConstraintsIterations,
        repeatLosingConstraintsQuads,
        repeatLosingConstraintsNonQuads,
        repeatLosingConstraintsAlign,
        feasibilityFix,
        hardParityConstraint,
        timeLimit,
        gapLimit,
        callbackTimeLimit,
        callbackGapLimit,
        gap,
        status,
        result);

    std::cout << "Gap: " << gap << std::endl;
    std::cout << "mingap: " << minimumGap << std::endl;
    if (status == ILPStatus::SOLUTIONFOUND && gap < minimumGap) {
        std::cout << "Solution found! Gap: " << gap << std::endl;
        ilpResults = result;
        return ilp_result;
    }
    else if (status == ILPStatus::SOLUTIONWRONG && !hardParityConstraint) {
        std::cout << std::endl << " >>>>>> Solution wrong! Trying with hard constraints for parity. It should not happen..." << std::endl << std::endl;

        return findSubdivisions(
            chartData,
            chartEdgeLength,
            method,
            alpha,
            isometry,
            regularityQuadrilaterals,
            regularityNonQuadrilaterals,
            regularityNonQuadrilateralsWeight,
            alignSingularities,
            alignSingularitiesWeight,
            repeatLosingConstraintsIterations,
            repeatLosingConstraintsQuads,
            repeatLosingConstraintsNonQuads,
            repeatLosingConstraintsAlign,
            feasibilityFix,
            true,
            timeLimit,
            gapLimit,
            callbackTimeLimit,
            callbackGapLimit,
            minimumGap,
            gap,
            ilpResults);
    }
    else if (method != ILPMethod::ABS ||
             alignSingularities ||
             repeatLosingConstraintsIterations > 0 ||
             repeatLosingConstraintsQuads ||
             repeatLosingConstraintsNonQuads ||
             repeatLosingConstraintsAlign)
    {
        std::cout << std::endl << " >>>>>> Minimum gap has been not reached. Trying with ABS (linear optimization method), singularity align disabled, minimum gap 1.0 and timeLimit x10. Gap was: " << gap << std::endl << std::endl;

        return findSubdivisions(
            chartData,
            chartEdgeLength,
            ILPMethod::ABS,
            alpha,
            isometry,
            regularityQuadrilaterals,
            regularityNonQuadrilaterals,
            regularityNonQuadrilateralsWeight,
            false,
            alignSingularitiesWeight,
            0,
            false,
            false,
            false,
            feasibilityFix,
            hardParityConstraint,
            timeLimit*10,
            gapLimit,
            callbackTimeLimit,
            callbackGapLimit,
            1.0,
            gap,
            ilpResults);
    } else {
        assert(false);
        return {};
    }
}

}
