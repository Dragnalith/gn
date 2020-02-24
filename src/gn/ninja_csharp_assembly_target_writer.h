// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_NINJA_CSHARP_ASSEMBLY_TARGET_WRITER_H_
#define TOOLS_GN_NINJA_CSHARP_ASSEMBLY_TARGET_WRITER_H_

#include "base/macros.h"
#include "gn/ninja_binary_target_writer.h"
#include "gn/csharp_tool.h"

struct EscapeOptions;

// Writes a .ninja file for a binary target type (an executable, a shared
// library, or a static library).
class NinjaCSharpAssemblyTargetWriter : public NinjaBinaryTargetWriter {
 public:
  NinjaCSharpAssemblyTargetWriter(const Target* target, std::ostream& out, std::ostream& csproj_out, std::ostream& csproj_sln_out_);
  ~NinjaCSharpAssemblyTargetWriter() override;

  void Run() override;

 private:
  void GenerateCSProj(std::vector<OutputFile>& deps, std::ostream& out, bool with_ninja_target);
  std::ostream& csproj_out_;
  std::ostream& csproj_sln_out_;
  const CSharpTool* msbuild_tool_;
  DISALLOW_COPY_AND_ASSIGN(NinjaCSharpAssemblyTargetWriter);
};


#endif  // TOOLS_GN_NINJA_CSHARP_ASSEMBLY_TARGET_WRITER_H_
