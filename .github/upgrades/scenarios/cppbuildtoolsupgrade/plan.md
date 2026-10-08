# CppBuildToolsUpgrade Plan

## Overview

Target: Resolve build failure caused by missing native dependencies required by RealSenseTracking and verify full rebuild succeeds.
Scope: Single-solution fix focused on native dependencies (vcpkg packages and project property TrackingDepsDir). Minimal source changes expected (project props update if path is set).

## Tasks

### 01-install-vcpkg-and-packages

Description:
- Ensure vcpkg is available on the build machine. If not present, clone and bootstrap vcpkg into a chosen local directory. Then use vcpkg to install the required native packages used by the project: `librealsense2` and `opencv4` (triplet: x64-windows) or the triplet that matches the solution's configuration.

Affected items:
- Solution: C:\Users\vmlab\source\repos\3D_TimeDivision_ParallaxBarrier_with_LenticularLens-master--tatebarrior\ProjectPB.sln
- Props file that checks the dependency: C:\Users\vmlab\source\repos\3D_TimeDivision_ParallaxBarrier_with_LenticularLens-master--tatebarrior\ProjectPB\RealSenseTracking.props

Done when:
- vcpkg is installed and `vcpkg list` shows `librealsense2` and `opencv4` for the chosen triplet, and an incremental build (cppupgrade_build_and_get_issues) no longer reports the original "Install vcpkg dependencies or set TrackingDepsDir" error.

---

### 02-set-TrackingDepsDir

Description:
- Configure the project to find the installed vcpkg packages by setting `TrackingDepsDir` to the vcpkg installed directory for the chosen triplet. Options:
  - Set an environment variable `TrackingDepsDir` on the build machine (temporary for CI/local session), or
  - Edit RealSenseTracking.props to point to the known vcpkg `installed\<triplet>` location (this requires unloading/reloading the project if editing .vcxproj/.props files).
- Preference: prefer setting an environment variable or MSBuild user properties to avoid committing machine-local absolute paths. If the project requires a committed change, we will propose editing RealSenseTracking.props and validate with cppupgrade_validate_vcxproj_file before reloading.

Affected items:
- C:\Users\vmlab\source\repos\3D_TimeDivision_ParallaxBarrier_with_LenticularLens-master--tatebarrior\ProjectPB\RealSenseTracking.props

Done when:
- A subsequent incremental build reports that the include paths exist and the previous Error at RealSenseTracking.props line 28 is resolved.

---

### 03-rebuild-and-verify

Description:
- Perform a full rebuild (`cppupgrade_rebuild_and_get_issues`) of the solution to verify no remaining errors/warnings introduced by the changes. If new issues appear, investigate and either fix or return to planning to add tasks.

Done when:
- Final full rebuild reports 0 errors and 0 warnings.

---

Notes:
- We will run `cppupgrade_build_and_get_issues` after each task change to verify progress and catch cascading issues early.
- No commits will be made to the repository until a working branch is created and you explicitly allow committing; per scenario defaults we will use branch `CppBuildToolsUpgrade-1` and commit strategy "After Each Task" unless you instruct otherwise.

