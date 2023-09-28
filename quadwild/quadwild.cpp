//***************************************************************************/
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

#include <iomanip>
#include <clocale>

#include "functions.h"
#ifdef _WIN32
#  include <windows.h>
#  include <stdlib.h>
#  include <errhandlingapi.h>
#endif

int actual_main(int argc, char* argv[])
{
#ifdef _WIN32
    // This make crashes very visible - without them, starting the
    // application from cmd.exe or powershell can surprisingly hide
    // any signs of a an application crash!
    SetErrorMode(0);
#endif
    Parameters parameters;

    FieldTriMesh trimesh;
    TraceMesh traceTrimesh;

    TriangleMesh trimeshToQuadrangulate;
    std::vector<std::vector<size_t>> trimeshPartitions;
    std::vector<std::vector<size_t>> trimeshCorners;
    std::vector<std::pair<size_t,size_t> > trimeshFeatures;
    std::vector<size_t> trimeshFeaturesC;

    PolyMesh quadmesh;
    std::vector<std::vector<size_t>> quadmeshPartitions;
    std::vector<std::vector<size_t>> quadmeshCorners;
    std::vector<int> ilpResult;

    if (argc < 2)
    {
        std::cerr << "usage: " << argv[0]
                  << " <mesh.{obj,ply}>"
                     " [1|2|3]"
                     " [*.sharp|*.rosy|*.txt]....\n"
                     " The 1|2|3 parameter determines after which step to stop:\n"
                     "   1: Remesh and field\n"
                     "   2: Tracing\n"
                     "   3: Quadrangulation (default)\n";
        return 1;
    }

    std::string meshFilename=argv[1];
    if (meshFilename.size() < 4) {
        std::cerr << "invalid mesh filename '" << meshFilename << "'" << std::endl;
        return 1;
    }
    auto meshFilenamePrefix = std::string(meshFilename.begin(), meshFilename.end() - 4);

    int stopAfterStep = 3;
    if (argc >= 2) {
        stopAfterStep = std::atoi(argv[2]);
        if (stopAfterStep < 1 || stopAfterStep > 3) {
            std::cerr << "unknown step '" << argv[2]
                << "' to stop after. valid: 1, 2, 3" << std::endl;
            return 1;
        }
    }


    std::string sharpFilename;
    std::string fieldFilename;

    //Use "." as decimal separator
    std::setlocale(LC_NUMERIC, "en_US.UTF-8");

    std::cout<<"Reading input..."<<std::endl;

    std::string configFile = "basic_setup.txt";
    for (int i=3;i<argc;i++)
    {
        int position;

        std::string pathTest=std::string(argv[i]);

        position=pathTest.find(".sharp");
        if (position!=-1)
        {
            sharpFilename=pathTest;
            parameters.hasFeature=true;
            continue;
        }

        position=pathTest.find(".txt");
        if (position!=-1)
        {
           configFile = pathTest;
           loadConfigFile(pathTest.c_str(), parameters);
           continue;
        }

        position=pathTest.find(".rosy");
        if (position!=-1)
        {
           fieldFilename=pathTest;
           parameters.hasField=true;
           continue;
        }
        std::cerr << "don't know what to do with optional parameter '"
                  << pathTest
                  << "'. Please supply .sharp, .txt or .rosy files."
                  << std::endl;
        return 1;
    }
    loadConfigFile(configFile, parameters);


    std::cout<<"Loading:"<<meshFilename.c_str()<<std::endl;

    bool allQuad;
    bool loaded=trimesh.LoadTriMesh(meshFilename,allQuad);
    trimesh.UpdateDataStructures();

    if (!loaded)
    {
        std::cout<<"Wrong mesh filename"<<std::endl;
        exit(0);
    }

    std::cout<<"Loaded "<<trimesh.fn<<" faces and "<<trimesh.vn<<" vertices"<<std::endl;

    std::cout<<std::endl<<"--------------------- 1 - Remesh and field ---------------------"<<std::endl;
    remeshAndField(trimesh, parameters, meshFilename, sharpFilename, fieldFilename);
    if (stopAfterStep == 1) {
        return 0;
    }

    std::cout<<std::endl<<"--------------------- 2 - Tracing ---------------------"<<std::endl;

    meshFilenamePrefix += "_rem";
    trace(meshFilenamePrefix, traceTrimesh);
    if (stopAfterStep == 2) {
        return 0;
    }

    std::cout<<std::endl<<"--------------------- 3 - Quadrangulation ---------------------"<<std::endl;
    quadrangulate(meshFilenamePrefix + ".obj", trimeshToQuadrangulate, quadmesh, trimeshPartitions, trimeshCorners, trimeshFeatures, trimeshFeaturesC, quadmeshPartitions, quadmeshCorners, ilpResult, parameters);
}


int main(int argc, char* argv[])
{
    try {
        return actual_main(argc, argv);
    }
    catch (std::runtime_error& e) {
        std::cerr << "fatal error: " << e.what() << std::endl;
        return 1;
    }
}
