This document are my personal note while I am trying to integrate C#/WPF to gn

# 24 Fev 2020
- I have duplicate rust_* related source file to create some csharp_*. I am not sure if everything is useful. The important change is happening in ninja_csharp_assembly_writer file, this is where write the ninja build line to generate the project. I am able to generate two build line, one to generate the .csproj, another one to compile. I have one problem to solves:
  How to specify the path to the .csproj to be generated for the 'csharp_generator' rule. This need to happen on the toolchain ninja file and not on the target ninja file. The toolchain need to use a substitution to go from {{csproj_file}} to $csproj_file. And the $csproj_file need to be defined on the target ninja file.
- I also need to figure out if it is possible to specify the source file type. The generator can map .cs to C# file, .xaml to XAML, and the rest as resources, but what if we want to be more precise for the resource and want to be able to change between 'Resource', 'Embedded Resource', etc. ?
- I also need to figure out how to forward build flag to the generator
- I also need to specify source for the generator without ninja considering those files as sources (they list of file cannot be a regular GN's {{source}} substitution)
- If .csproj are generated during the build by ninja and not by GN it will be hard to include them in the visual studio solution.
# 25 Fev 2020
- I have succeeded in generating .csproj with GN