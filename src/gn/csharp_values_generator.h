// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_CSHARP_VALUES_GENERATOR_H_
#define TOOLS_GN_CSHARP_VALUES_GENERATOR_H_

#include "base/macros.h"
#include "gn/target.h"

class FunctionCallNode;

// Collects and writes specified data.
class CSharpTargetGenerator {
 public:
  CSharpTargetGenerator(Target* target,
                      Scope* scope,
                      const FunctionCallNode* function_call,
                      Err* err);
  ~CSharpTargetGenerator();

  void Run();

 private:
  bool FillProjectPath();
  bool FillOutputTypeAndExtension();
  bool FillProjectGuid();

  Target* target_;
  Scope* scope_;
  const FunctionCallNode* function_call_;
  Err* err_;

  DISALLOW_COPY_AND_ASSIGN(CSharpTargetGenerator);
};

#endif  // TOOLS_GN_CSHARP_VALUES_GENERATOR_H_
