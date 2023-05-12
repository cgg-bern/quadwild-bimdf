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

#include "qr_ilp.h"
#include <stdexcept>

namespace QuadRetopology {
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
        std::vector<int>& ilpResults)
{
    throw std::runtime_error("libQuadRetopology / QuadWild was built without Gurobi. Please select another solver.");
}

}
}


