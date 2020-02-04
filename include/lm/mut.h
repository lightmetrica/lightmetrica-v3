/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/component.h>
#include "bidir.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// Subspace type
// This type is used to identify the subspace
// selected in the mutation process.
using Subspace = std::tuple<int, int, int>;

// Proposal state
struct Proposal {
    Path path;
    Subspace subspace;
};

// Interface for mutation strategies
class Mut : public Component {
public:
    // Check if the current state is mutatable with the selected strategy
    virtual bool check_mutatable(const Path& curr) const = 0;

    // Mutate the current state and generate proposal state
    virtual std::optional<Proposal> sample_proposal(Rng& rng, const Path& curr) const = 0;

    // Reverse the subspace
    virtual Subspace reverse_subspace(const Subspace& subspace) const = 0;

    // Evaluate Q(y|x) := T(y|x)/f(y).
    // This function is used to compute acceptance ratio a(y|x) := min(1, Q(x|y)/Q(y|x)).
    virtual Float eval_Q(const Path& x, const Path& y, const Subspace& subspace) const = 0;
};

LM_NAMESPACE_END(LM_NAMESPACE)
