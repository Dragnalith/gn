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
    std::ostream& csproj_out,
    std::ostream& csproj_sln_out)
    : NinjaBinaryTargetWriter(target, out),
      csproj_out_(csproj_out),
      csproj_sln_out_(csproj_sln_out),
      msbuild_tool_(
          target->toolchain()->GetToolForTargetFinalOutputAsCSharp(target)) {}

NinjaCSharpAssemblyTargetWriter::~NinjaCSharpAssemblyTargetWriter() = default;

void NinjaCSharpAssemblyTargetWriter::Run() {
  out_ << "build ";
  path_output_.WriteFile(out_, target_->dependency_output_file());

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

  GenerateCSProj(deps, csproj_out_, false);
  GenerateCSProj(deps, csproj_sln_out_, true);
}

void NinjaCSharpAssemblyTargetWriter::GenerateCSProj(
    std::vector<OutputFile>& deps, std::ostream& out,
    bool with_ninja_build) {
  std::string target_name = target_->GetComputedOutputName();
  SourceDir project_dir = target_->csharp_values().project_path().GetDir();
  SourceDir target_dir = target_->label().dir();
  SourceDir build_dir = target_->settings()->build_settings()->build_dir();
  SourceDir output_dir =
      target_->dependency_output_file()
          .AsSourceFile(target_->settings()->build_settings())
          .GetDir();

  PathOutput project_path_output(
      GetBuildDirForTargetAsSourceDir(target_, BuildDirType::OBJ),
      target_->settings()->build_settings()->root_path_utf8(),
      EscapingMode::ESCAPE_NONE);

  out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
  XmlElementWriter project(
      out, "Project",
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
    commonPropertyGroup->SubElement("RootBuildDir")->Text("$(SolutionDir)");
    commonPropertyGroup->SubElement("ProjectGuid")
        ->Text(target_->csharp_values().project_guid());
    commonPropertyGroup->SubElement("OutputType")
        ->Text(target_->csharp_values().assembly_type());
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
        auto hint = ref->SubElement("HintPath");
        hint->StartContent(false);
        if (l.is_system_absolute()) {
          out << RebasePath(l.value(), project_dir);
        } else {
          project_path_output.WriteFile(out, l);
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
    std::set<SourceFile> sources;
    for (const auto& source : target_->sources()) {
      sources.insert(source);
    }
    auto itemGroup = project.SubElement("ItemGroup");
    for (const auto& source : sources) {
      switch (source.type()) {
        case SourceFile::SOURCE_CS:
          {
            auto compile = itemGroup->SubElement(
                "Compile", "Include",
                SourceFileWriter(project_path_output, source));
            compile->SubElement("Link")->Text(RebasePath(source.value(), target_dir));

            std::string val = source.value();
            if (base::EndsWith(val, ".xaml.cs", base::CompareCase::SENSITIVE)) {
              SourceFile baseSource(val.substr(0, val.size() - 3));
              if (sources.find(baseSource) != sources.end()) {
                compile->SubElement("DependentUpon")->Text(RebasePath(baseSource.value(), target_dir));
                compile->SubElement("SubType")->Text("Code");
              }
            }
          }
          break;
        case SourceFile::SOURCE_XAML:
          {
            auto page = itemGroup->SubElement("Page", "Include", SourceFileWriter(project_path_output, source));
            page->SubElement("Link")->Text(RebasePath(source.value(), target_dir));
            page->SubElement("Generator")->Text("MSBuild:Compile");
            page->SubElement("SubType")->Text("Designer");
          }
          break;
        default:
          {
            auto resource = itemGroup->SubElement("Resource", "Include", SourceFileWriter(project_path_output, source));
            resource->SubElement("Link")->Text(RebasePath(source.value(), target_dir));
          }
      }
    }
  }
  project.SubElement(
      "Import", XmlAttributes("Project",
                              "$(MSBuildToolsPath)\\Microsoft.CSharp.targets"));
  if (with_ninja_build) {
      PathOutput ninja_path_output_(
          target_->settings()->build_settings()->build_dir(),
          target_->settings()->build_settings()->root_path_utf8(),
          EscapingMode::ESCAPE_NINJA_COMMAND);
      std::ostringstream ninja_target_out;
      DCHECK(!target_->dependency_output_file().value().empty());
      ninja_path_output_.WriteFile(ninja_target_out,
                                   target_->dependency_output_file());
      std::string ninja_target = ninja_target_out.str();
      if (ninja_target.compare(0, 2, "./") == 0) {
        ninja_target = ninja_target.substr(2);
      }
      {
        std::unique_ptr<XmlElementWriter> build =
            project.SubElement("Target", XmlAttributes("Name", "Build"));
        build->SubElement(
            "Exec",
            XmlAttributes("Command", "call ninja.exe -C $(RootBuildDir) " + ninja_target));
      }

      {
        std::unique_ptr<XmlElementWriter> clean =
            project.SubElement("Target", XmlAttributes("Name", "Clean"));
        clean->SubElement(
            "Exec",
            XmlAttributes("Command",
                          "call ninja.exe -C $(RootBuildDir) -tclean " + ninja_target));
      }
  }
}
