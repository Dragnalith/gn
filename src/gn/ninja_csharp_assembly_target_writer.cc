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
#include "gn/xml_element_writer.h"

namespace {

struct SourceFileWriter {
  SourceFileWriter(PathOutput& path_output, const SourceFile& source_file)
      : path_output_(path_output), source_file_(source_file) {}
  ~SourceFileWriter() = default;

  void operator()(std::ostream& out) const {
    path_output_.WriteFile(out, source_file_);
  }

  PathOutput& path_output_;
  const SourceFile& source_file_;
};

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
      msbuild_tool_(
          target->toolchain()->GetToolForTargetFinalOutputAsCSharp(target)) {}

NinjaCSharpAssemblyTargetWriter::~NinjaCSharpAssemblyTargetWriter() = default;

void NinjaCSharpAssemblyTargetWriter::Run() {
  out_ << "build ";
  path_output_.WriteFile(out_, target_->dependency_output_file());
  std::string target_name = target_->GetComputedOutputName();

  std::vector<OutputFile> deps;
  for (const auto& pair : target_->GetDeps(Target::DEPS_LINKED)) {
    const Target* dep = pair.ptr;
    if (dep->output_type() == Target::CSHARP_ASSEMBLY) {
      deps.push_back(dep->dependency_output_file());
    }
  }

  out_ << ": " << rule_prefix_ << msbuild_tool_->name();
  out_ << " ";
  path_output_.WriteFile(out_, target_->csharp_values().project_path());
  out_ << " |";
  for (const auto& source : target_->sources()) {
    out_ << " ";
    path_output_.WriteFile(out_, source);
  }
  for (const auto& source : deps) {
    out_ << " ";
    path_output_.WriteFile(out_, source);
  }

  out_ << std::endl;

  SourceDir project_dir = target_->csharp_values().project_path().GetDir();
  SourceDir target_dir = target_->label().dir();
  SourceDir build_dir = target_->settings()->build_settings()->build_dir();
  SourceDir output_dir =
      target_->dependency_output_file()
          .AsSourceFile(target_->settings()->build_settings())
          .GetDir();

#if 0
  csproj_out_ << "proj_dir: " << project_dir.value() << '\n';
  csproj_out_ << "out_dir: " << output_dir.value() << '\n';
  csproj_out_ << "target_dir" << target_dir.value() << '\n';
  for (const auto& source : target_->sources()) {
    csproj_out_ << "source: ";
    path_output_.WriteFile(csproj_out_, source);
    csproj_out_ << '\n';
    csproj_out_ << "link: " << RebasePath(source.value(), target_dir) << '\n';
  }
  csproj_out_ << "output: " << output_files[0].value();
#endif
  PathOutput project_path_output(
      GetBuildDirForTargetAsSourceDir(target_, BuildDirType::OBJ),
      target_->settings()->build_settings()->root_path_utf8(),
      EscapingMode::ESCAPE_NONE);

  csproj_out_ << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
  XmlElementWriter project(
      csproj_out_, "Project",
      XmlAttributes("ToolsVersion", "15.0")
          .add("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003"));

  project.SubElement(
      "Import", XmlAttributes("Project",
                              "$(MSBuildExtensionsPath)\\$(MSBuildToolsVersion)"
                              "\\Microsoft.Common.props")
                    .add("Condition",
                         "Exists('$(MSBuildExtensionsPath)\\$("
                         "MSBuildToolsVersion)\\Microsoft.Common.props')"));

  {
    auto commonPropertyGroup = project.SubElement("PropertyGroup");
    commonPropertyGroup->SubElement("Platform")->Text("AnyCPU");
    commonPropertyGroup->SubElement("ProjectGuid")
        ->Text("{696F6F3E-15C6-4DB8-ABE9-1FC7641E1B9F}");
    commonPropertyGroup->SubElement("OutputType")
        ->Text(target_->csharp_values().output_type());
    commonPropertyGroup->SubElement("RootNamespace")->Text(target_name);
    commonPropertyGroup->SubElement("AssemblyName")->Text(target_name);
    commonPropertyGroup->SubElement("TargetFrameworkVersion")->Text("v4.6.1");
    commonPropertyGroup->SubElement("FileAlignment")->Text("512");
    commonPropertyGroup->SubElement("AutoGenerateBindingRedirects")
        ->Text("true");
  }
  {
    auto propertyGroup = project.SubElement("PropertyGroup");
    propertyGroup->SubElement("PlatformTarget")->Text("AnyCPU");
    propertyGroup->SubElement("DebugSymbols")->Text("true");
    propertyGroup->SubElement("Optimize")->Text("false");
    propertyGroup->SubElement("OutputPath")
        ->Text(RebasePath(output_dir.value(), project_dir));
    propertyGroup->SubElement("DefineConstants")->Text("DEBUG;TRACE");
    propertyGroup->SubElement("ErrorReport")->Text("prompt");
    propertyGroup->SubElement("WarningLevel")->Text("4");
  }
  {
    auto itemGroup = project.SubElement("ItemGroup");

    std::set<SourceFile> libs;
    for (ConfigValuesIterator iter(target_); !iter.done(); iter.Next()) {
      for (const auto& l : iter.cur().cs_references()) {
        libs.insert(l);
      }
    }
    for (const auto& d : deps) {
      libs.insert(d.AsSourceFile(target_->settings()->build_settings()));
    }
    for (const auto& l : libs) {
      base::FilePath output = UTF8ToFilePath(l.value());
      {
        auto ref = itemGroup->SubElement(
            "Reference",
            XmlAttributes("Include",
                          output.RemoveExtension().BaseName().As8Bit()));
        auto hint = ref->SubElement("Hint");
        hint->StartContent(false);
        if (l.is_system_absolute()) {
          csproj_out_ << RebasePath(l.value(), project_dir);
        } else {
          project_path_output.WriteFile(csproj_out_, l);
        }
      }
    }

    std::set<std::string> references;
    for (ConfigValuesIterator iter(target_); !iter.done(); iter.Next()) {
      for (const auto& s : iter.cur().cs_system_references()) {
        references.insert(s);
      }
    }
    for (const auto& ref : references) {
      itemGroup->SubElement("Reference", XmlAttributes("Include", ref));
    }
  }
  {
    auto itemGroup = project.SubElement("ItemGroup");
    for (const auto& source : target_->sources()) {
      itemGroup
          ->SubElement("Compile", "Include",
                       SourceFileWriter(project_path_output, source))
          ->SubElement("Link")
          ->Text(RebasePath(source.value(), target_dir));
    }
  }
  project.SubElement(
      "Import", XmlAttributes("Project",
                              "$(MSBuildToolsPath)\\Microsoft.CSharp.targets"));
}
