// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_CSHARP_TARGET_VALUES_H_
#define TOOLS_GN_CSHARP_TARGET_VALUES_H_

#include <map>

#include "base/containers/flat_map.h"
#include "gn/label.h"
#include "gn/source_file.h"

// Holds the values (outputs, args, script name, etc.) for either an action or
// an action_foreach target.
class CSharpValues {
 public:
  CSharpValues();
  ~CSharpValues();

  // Name of generated csproj
  std::string& project_name() { return project_name_; }
  const std::string& project_name() const { return project_name_; }

 private:
  std::string project_name_;

  DISALLOW_COPY_AND_ASSIGN(CSharpValues);
};

#endif  // TOOLS_GN_CSHARP_TARGET_VALUES_H_
