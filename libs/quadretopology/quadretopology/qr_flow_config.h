#include "libsatsuma/Extra/Highlevel.hh"
#include <nlohmann/json.hpp>
#include <libsatsuma/Problems/BiMDF.hh>
#include <libsatsuma/Extra/json.hh>

namespace QuadRetopology {

enum class PairedHalfTarget {
    Half,
    Simple,
};
NLOHMANN_JSON_SERIALIZE_ENUM( PairedHalfTarget, {
    {PairedHalfTarget::Half, "half"},
    {PairedHalfTarget::Simple, "simple"},
})

enum class ObjectiveKind {
    AbsDeviation,
    QuadraticDeviation,
};
NLOHMANN_JSON_SERIALIZE_ENUM( ObjectiveKind, {
    {ObjectiveKind::AbsDeviation, "abs"},
    {ObjectiveKind::QuadraticDeviation, "quad"},
})


struct FlowConfigPaired{
    double iso_weight = .5;
    ObjectiveKind iso_objective = ObjectiveKind::AbsDeviation;
    double unalign_weight = 1.;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FlowConfigPaired,
                                   iso_weight,
                                   iso_objective,
                                   unalign_weight)

struct FlowConfig {
    PairedHalfTarget paired_half_target = PairedHalfTarget::Half;
    bool paired_resolve_new_targets = false;
    FlowConfigPaired paired_initial;
    FlowConfigPaired paired_resolve;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FlowConfig,
                                   paired_half_target,
                                   paired_resolve_new_targets,
                                   paired_initial,
                                   paired_resolve)

} // namespace QuadRetopology
