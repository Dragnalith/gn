// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/csharp_variables.h"

namespace variables {

// CSharp target variables ------------------------------------------------------


const char kCSharpProjectName[] = "project_name";
const char kCSharpProjectName_HelpShort[] =
    "project_name: [string] The name for the underlying generated csproj file.";
const char kCSharpProjectName_Help[] =
    R"(project_name: [string] The name for the underlying generated csproj file.

  Valid for `csharp_assembly`.

  If project_name is not set, then this rule will use the target name.
)";

void InsertCSharpVariables(VariableInfoMap* info_map) {
  info_map->insert(std::make_pair(
      kCSharpProjectName,
      VariableInfo(kCSharpProjectName_HelpShort, kCSharpProjectName_Help)));
}

}  // namespace variables
