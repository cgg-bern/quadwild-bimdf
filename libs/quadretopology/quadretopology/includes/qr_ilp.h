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

#ifndef QR_ILP_H
#define QR_ILP_H

#include "qr_charts.h"
#include "qr_parameters.h"
#include <nlohmann/json.hpp>

#define MIN_SUBDIVISION_VALUE 1

#define ILP_FIND_SUBDIVISION -1
#define ILP_IGNORE -2           // subside belongs to another cluster

namespace QuadRetopology {

struct ILPStats {
    double support_obj;
    double obj;
    double cost_isometry;
    double cost_regularity;
    double cost_alignment;
    double cost_alignment_full; // including dropped constraints
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ILPStats,
                                   support_obj,
                                   obj,
                                   cost_isometry,
                                   cost_regularity,
                                   cost_alignment,
                                   cost_alignment_full);

struct ILPResult {
    std::vector<ILPStats> stats;
};

enum ILPStatus { SOLUTIONFOUND, SOLUTIONWRONG, INFEASIBLE };

namespace internal {

ILPResult solveILP(
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
        double& gap,
        ILPStatus& status,
        std::vector<int>& ilpResults);

}
}

//#include "qr_ilp.cpp"

#endif // QR_ILP_H
