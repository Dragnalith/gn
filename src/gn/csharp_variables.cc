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


const char kCSharpProjectGuid[] = "project_guid";
const char kCSharpProjectGuid_HelpShort[] =
    "project_guid: [string] The GUID to be used by the underlying .csproj generated file";
const char kCSharpProjectGuid_Help[] =
    R"(project_guid: [string] The GUID to be used by the underlying .csproj generated file.

  Valid for `csharp_assembly`.

  If project_guid is not set, it will be generated.
)";

void InsertCSharpVariables(VariableInfoMap* info_map) {
  info_map->insert(std::make_pair(
      kCSharpAssemblyType,
      VariableInfo(kCSharpAssemblyType_HelpShort, kCSharpAssemblyType_Help)));
  info_map->insert(std::make_pair(
      kCSharpProjectGuid,
      VariableInfo(kCSharpProjectGuid_HelpShort, kCSharpProjectGuid_Help)));
}

}  // namespace variables
