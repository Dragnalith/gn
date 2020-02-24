// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/csharp_values_generator.h"

#include "gn/config_values_generator.h"
#include "base/strings/string_util.h"
#include "gn/err.h"
#include "gn/filesystem_utils.h"
#include "gn/functions.h"
#include "gn/parse_tree.h"
#include "gn/csharp_variables.h"
#include "gn/scope.h"
#include "gn/target.h"
#include "gn/value_extractors.h"
#include "gn/substitution_writer.h"
#include "gn/visual_studio_utils.h"

CSharpTargetGenerator::CSharpTargetGenerator(Target* target,
                                         Scope* scope,
                                         const FunctionCallNode* function_call,
                                         Err* err)
    : target_(target),
      scope_(scope),
      function_call_(function_call),
      err_(err) {}

CSharpTargetGenerator::~CSharpTargetGenerator() = default;

void CSharpTargetGenerator::Run() {
  // Check that this type of target is Rust-supported.
  if (target_->output_type() != Target::CSHARP_ASSEMBLY) {
    // Only valid rust output types.
    *err_ = Err(function_call_,
                "Target type \"" +
                    std::string(Target::GetStringForOutputType(
                        target_->output_type())) +
                    "\" is not supported for CSharp compilation.",
                "Supported target types are \"csharp_assembly\".");
    return;
  }

  if (!FillOutputTypeAndExtension())
    return;
  if (!FillProjectPath())
    return;
  if (!FillProjectGuid())
    return;
}

bool CSharpTargetGenerator::FillProjectPath() {
  SourceFile projectPath(
      GetBuildDirForTargetAsSourceDir(target_, BuildDirType::OBJ).value() +
      target_->label().name() + ".csproj");

  target_->csharp_values().set_project_path(projectPath);
  return true;
}

bool CSharpTargetGenerator::FillProjectGuid() {
  const Value* value = scope_->GetValue(variables::kCSharpProjectGuid, true);
  if (!value) {
    // The target name will be used.
    target_->csharp_values().project_guid() = MakeGuid(target_->csharp_values().project_path().value(), "csproj");
    return true;
  }
  if (!value->VerifyTypeIs(Value::STRING, err_))
    return false;

  target_->csharp_values().project_guid() = std::move(value->string_value());

  return true;
}

bool CSharpTargetGenerator::FillOutputTypeAndExtension() {
  const Value* value = scope_->GetValue(variables::kCSharpAssemblyType, true);
  if (!value) {
    // The target name will be used.
    target_->csharp_values().assembly_type() = "exe";
    target_->csharp_values().extension() = ".exe";
    return true;
  }
  if (!value->VerifyTypeIs(Value::STRING, err_))
    return false;

  target_->csharp_values().assembly_type() = std::move(value->string_value());
  std::string assembly_type = target_->csharp_values().assembly_type();
  if (base::EqualsCaseInsensitiveASCII(assembly_type, "exe") || base::EqualsCaseInsensitiveASCII(assembly_type, "winexe")) {
    target_->csharp_values().extension() = ".exe";
  } else {
    target_->csharp_values().extension() = ".dll";
  }

  return true;
}