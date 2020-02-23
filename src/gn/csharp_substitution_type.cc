// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/csharp_substitution_type.h"

#include <stddef.h>
#include <stdlib.h>

#include "gn/err.h"
#include "gn/substitution_type.h"

const SubstitutionTypes CSharpSubstitutions = {
    &kCSharpSubstitutionProjectName,
};

// Valid for Rust tools.
const Substitution kCSharpSubstitutionProjectName = {"{{project_name}}",
                                                 "project_name"};

bool IsValidCSharpSubstitution(const Substitution* type) {
  return IsValidToolSubstitution(type) || IsValidSourceSubstitution(type) ||
         type == &SubstitutionOutputDir ||
         type == &SubstitutionOutputExtension ||
         type == &kCSharpSubstitutionProjectName;
}
