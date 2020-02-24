// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/csharp_variables.h"

namespace variables {

// CSharp target variables ------------------------------------------------------


const char kCSharpAssemblyType[] = "assembly_type";
const char kCSharpAssemblyType_HelpShort[] =
    "assembly_type: [string] The assembly type ('exe', 'winexe', 'library') for the C# assembly.";
const char kCSharpAssemblyType_Help[] =
    R"(assembly_type: [string] The assembly type ('exe', 'winexe', 'library') for the C# assembly.

  Valid for `csharp_assembly`.

  If assembly_type is not set, then this rule will use 'exe'.
)";

void InsertCSharpVariables(VariableInfoMap* info_map) {
  info_map->insert(std::make_pair(
      kCSharpAssemblyType,
      VariableInfo(kCSharpAssemblyType_HelpShort, kCSharpAssemblyType_Help)));
}

}  // namespace variables
