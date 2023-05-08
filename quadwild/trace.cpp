#include "trace.h"

#include <tracing/tracer_interface.h>

bool trace(const std::string& filename_prefix, TraceMesh& traceTrimesh)
{
    std::string meshFilename = filename_prefix + ".obj";
    std::string fieldFilename = filename_prefix + ".rosy";
    std::string sharpFilename = filename_prefix + ".sharp";

    std::cout<<"Loading Remeshed M:"<<meshFilename.c_str()<<std::endl;
    std::cout<<"Loading Rosy Field:"<<fieldFilename.c_str()<<std::endl;
    std::cout<<"Loading Sharp F:"<<sharpFilename.c_str()<<std::endl;

    //Mesh load
    printf("Loading the mesh \n");
    bool loadedMesh=traceTrimesh.LoadMesh(meshFilename);
    if (!loadedMesh) {
        std::cerr << "failed to load mesh from " << meshFilename << std::endl;
        return false;
    }
    traceTrimesh.UpdateAttributes();

    //Field load
    bool loadedField=traceTrimesh.LoadField(fieldFilename);
    if (!loadedField) {
        std::cerr << "failed to load field from " << fieldFilename << std::endl;
        return false;
    }
    traceTrimesh.UpdateAttributes();

    //Sharp load
    bool loadedFeatures=traceTrimesh.LoadSharpFeatures(sharpFilename);
    if (!loadedFeatures) {
        std::cerr << "failed to load features from " << sharpFilename << std::endl;
        return false;
    }
    traceTrimesh.SolveGeometricIssues();
    traceTrimesh.UpdateSharpFeaturesFromSelection();

    //preprocessing mesh
    PreProcessMesh(traceTrimesh);

    //initializing graph
    VertexFieldGraph<TraceMesh> VGraph(traceTrimesh);
    VGraph.InitGraph(false);

    //INIT TRACER
    typedef PatchTracer<TraceMesh> TracerType;
    TracerType PTr(VGraph);
    TraceMesh::ScalarType Drift=100;
    bool add_only_needed=true;
    bool final_removal=true;
    bool meta_mesh_collapse=true;
    bool force_split=false;
    PTr.sample_ratio=0.01;
    PTr.CClarkability=1;
    PTr.split_on_removal=true;
    PTr.away_from_singular=true;
    PTr.match_valence=true;
    PTr.check_quality_functor=false;
    PTr.MinVal=3;
    PTr.MaxVal=5;
    PTr.Concave_Need=1;

    //TRACING
    PTr.InitTracer(Drift,false);
    RecursiveProcess<TracerType>(PTr,Drift, add_only_needed,final_removal,true,meta_mesh_collapse,force_split,true,false);
    PTr.SmoothPatches();
    SaveAllData(PTr,filename_prefix,0,false,false);
    return true;
}
