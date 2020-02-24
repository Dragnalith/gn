// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/csharp_variables.h"

namespace variables {

// CSharp target variables ------------------------------------------------------


const char kCSharpOutputType[] = "output_type";
const char kCSharpOutputType_HelpShort[] =
    "output_type: [string] The output type ('Exe', 'WinExe', 'Library') for the C# assembly.";
const char kCSharpOutputType_Help[] =
    R"(output_type: [string] The output type ('Exe', 'WinExe', 'Library') for the C# assembly.

  Valid for `csharp_assembly`.

  If output_type is not set, then this rule will use 'Exe'.
)";

void InsertCSharpVariables(VariableInfoMap* info_map) {
  info_map->insert(std::make_pair(
      kCSharpOutputType,
      VariableInfo(kCSharpOutputType_HelpShort, kCSharpOutputType_Help)));
}

}  // namespace variables
