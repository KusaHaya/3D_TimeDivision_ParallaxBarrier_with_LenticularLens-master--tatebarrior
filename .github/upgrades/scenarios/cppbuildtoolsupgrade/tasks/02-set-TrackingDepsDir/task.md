# 02-set-TrackingDepsDir: set-TrackingDepsDir

Description:
- Configure the project to find the installed vcpkg packages by setting `TrackingDepsDir` to the vcpkg installed directory for the chosen triplet. Options:
  - Set an environment variable `TrackingDepsDir` on the build machine (temporary for CI/local session), or
  - Edit RealSenseTracking.props to point to the known vcpkg `installed\<triplet>` location (this requires unloading/reloading the project if editing .vcxproj/.props files).
- Preference: prefer setting an environment variable or MSBuild user properties to avoid committing machine-local absolute paths. If the project requires a committed change, we will propose editing RealSenseTracking.props and validate with cppupgrade_validate_vcxproj_file before reloading.

Affected items:
- C:\Users\vmlab\source\repos\3D_TimeDivision_ParallaxBarrier_with_LenticularLens-master--tatebarrior\ProjectPB\RealSenseTracking.props

Done when:
- A subsequent incremental build reports that the include paths exist and the previous Error at RealSenseTracking.props line 28 is resolved.
