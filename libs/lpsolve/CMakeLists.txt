project(lpsolve VERSION 5.5.2.11 LANGUAGES C)

add_library(lpsolve
    lp_solve_5.5/lp_presolve.c
    lp_solve_5.5/lp_price.c
    lp_solve_5.5/lp_crash.c
    lp_solve_5.5/lp_pricePSE.c
    lp_solve_5.5/shared/mmio.c
    lp_solve_5.5/shared/myblas.c
    lp_solve_5.5/shared/commonlib.c
    lp_solve_5.5/lp_mipbb.c
    lp_solve_5.5/lp_MDO.c
    lp_solve_5.5/lp_Hash.c
    lp_solve_5.5/lp_params.c
    lp_solve_5.5/ini.c
    lp_solve_5.5/yacc_read.c
    lp_solve_5.5/lp_utils.c
    lp_solve_5.5/lp_wlp.c
    lp_solve_5.5/lp_matrix.c
    lp_solve_5.5/fortify.c
    lp_solve_5.5/lp_lib.c
    lp_solve_5.5/lp_MPS.c
    lp_solve_5.5/lp_simplex.c
    lp_solve_5.5/lp_scale.c
    lp_solve_5.5/lp_report.c
    lp_solve_5.5/colamd/colamd.c
    lp_solve_5.5/lp_SOS.c
    lp_solve_5.5/lp_rlp.c
    #)
#add_library(lpsolve_bfp
    lp_solve_5.5/bfp/bfp_LUSOL/LUSOL/lusol.c
    lp_solve_5.5/bfp/bfp_LUSOL/lp_LUSOL.c
)

target_compile_definitions(lpsolve PRIVATE 
    "LoadInverseLib=FALSE"
    "LoadLanguageLib=FALSE"
    "LoadableBlasLib=0"
    "INVERSE_ACTIVE=INVERSE_LUSOL"
    "RoleIsExternalInvEngine"
    "NOFORTIFY"
    "CHECK_SOLUTION"
    "YY_NEVER_INTERACTIVE"
    "PARSER_LP"
    "LP_MAXLINELEN=0")

#target_compile_definitions(lpsolve PRIVATE "-DINVERSE_ACTIVE=INVERSE_LUSOL")
target_include_directories(lpsolve PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lp_solve_5.5/shared")
target_include_directories(lpsolve PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/lp_solve_5.5/")
target_include_directories(lpsolve PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/lp_solve_5.5/colamd")
target_include_directories(lpsolve PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/lp_solve_5.5/bfp/")
target_include_directories(lpsolve PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/lp_solve_5.5/bfp/bfp_LUSOL/")
target_include_directories(lpsolve PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/lp_solve_5.5/bfp/bfp_LUSOL/LUSOL")

#find_package(SuiteSparse REQUIRED COMPONENTS COLAMD)
#target_link_libraries(lpsolve SuiteSparse::COLAMD)

add_library(lpsolve::lpsolve ALIAS lpsolve)


