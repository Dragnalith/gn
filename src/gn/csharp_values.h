// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_CSHARP_TARGET_VALUES_H_
#define TOOLS_GN_CSHARP_TARGET_VALUES_H_

#include <map>

#include "base/containers/flat_map.h"
#include "gn/label.h"
#include "gn/source_file.h"
#include "gn/output_file.h"

// Holds the values (outputs, args, script name, etc.) for either an action or
// an action_foreach target.
class CSharpValues {
 public:
  CSharpValues();
  ~CSharpValues();

  // Name of generated csproj
  const SourceFile& project_path() const { return project_path_; }
  void set_project_path(const SourceFile& s) { project_path_ = s; }

  const std::string& output_type() const { return output_type_; }
  std::string& output_type() { return output_type_; }
  const std::string& extension() const { return extension_; }
  std::string& extension() { return extension_; }

 private:
  SourceFile project_path_;
  OutputFile output_path_;
  std::string extension_;
  std::string output_type_;

  DISALLOW_COPY_AND_ASSIGN(CSharpValues);
};

#endif  // TOOLS_GN_CSHARP_TARGET_VALUES_H_
