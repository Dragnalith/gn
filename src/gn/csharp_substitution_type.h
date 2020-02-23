// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_CSHARP_SUBSTITUTION_TYPE_H_
#define TOOLS_GN_CSHARP_SUBSTITUTION_TYPE_H_

#include <set>
#include <vector>

#include "gn/substitution_type.h"

// The set of substitutions available to CSharp tools.
extern const SubstitutionTypes CSharpSubstitutions;

// Valid for CSharp tools.
extern const Substitution kCSharpSubstitutionProjectName;

bool IsValidCSharpSubstitution(const Substitution* type);

#endif  // TOOLS_GN_CSHARP_SUBSTITUTION_TYPE_H_
