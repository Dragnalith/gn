// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_CSHARP_VARIABLES_H_
#define TOOLS_GN_CSHARP_VARIABLES_H_

#include "gn/variables.h"

namespace variables {

// CSharp target vars ------------------------------------------------------



extern const char kCSharpAssemblyType[];
extern const char kCSharpAssemblyType_HelpShort[];
extern const char kCSharpAssemblyType_Help[];



void InsertCSharpVariables(VariableInfoMap* info_map);

}  // namespace variables

#endif  // TOOLS_GN_CSHARP_VARIABLES_H_