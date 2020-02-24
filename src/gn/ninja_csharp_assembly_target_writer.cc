// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/ninja_csharp_assembly_target_writer.h"

#include <sstream>

#include "base/strings/string_util.h"
#include "gn/deps_iterator.h"
#include "gn/filesystem_utils.h"
#include "gn/general_tool.h"
#include "gn/ninja_target_command_util.h"
#include "gn/ninja_utils.h"
#include "gn/rust_substitution_type.h"
#include "gn/substitution_writer.h"
#include "gn/target.h"

namespace {

// Returns the proper escape options for writing compiler and linker flags.
EscapeOptions GetFlagOptions() {
  EscapeOptions opts;
  opts.mode = ESCAPE_NINJA_COMMAND;
  return opts;
}

void WriteVar(const char* name,
              const std::string& value,
              EscapeOptions opts,
              std::ostream& out) {
  out << name << " = ";
  EscapeStringToStream(out, value, opts);
  out << std::endl;
}

}  // namespace

NinjaCSharpAssemblyTargetWriter::NinjaCSharpAssemblyTargetWriter(
    const Target* target,
    std::ostream& out,
    std::ostream& csproj_out)
    : NinjaBinaryTargetWriter(target, out),
      csproj_out_(csproj_out),
      msbuild_tool_(target->toolchain()->GetToolForTargetFinalOutputAsCSharp(target)) {}

NinjaCSharpAssemblyTargetWriter::~NinjaCSharpAssemblyTargetWriter() = default;

void NinjaCSharpAssemblyTargetWriter::Run() {
  std::vector<OutputFile> output_files;
  SubstitutionWriter::ApplyListToLinkerAsOutputFile(
      target_, msbuild_tool_, msbuild_tool_->outputs(), &output_files);

  out_ << "build";
  path_output_.WriteFiles(out_, output_files);

  out_ << ": " << rule_prefix_ << msbuild_tool_->name();
  out_ << " ";
  path_output_.WriteFile(out_,
                         target_->csharp_values().project_path());
  out_ << " |";
  for (const auto& source : target_->sources()) {
    out_ << " ";
    path_output_.WriteFile(out_, source);
  }

  out_ << std::endl;
}
