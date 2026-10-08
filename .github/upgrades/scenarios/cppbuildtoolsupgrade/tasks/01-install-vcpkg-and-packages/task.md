# 01-install-vcpkg-and-packages: install-vcpkg-and-packages

Description:
- Ensure vcpkg is available on the build machine. If not present, clone and bootstrap vcpkg into a chosen local directory. Then use vcpkg to install the required native packages used by the project: `librealsense2` and `opencv4` (triplet: x64-windows) or the triplet that matches the solution's configuration.

Affected items:
- Solution: C:\Users\vmlab\source\repos\3D_TimeDivision_ParallaxBarrier_with_LenticularLens-master--tatebarrior\ProjectPB.sln
- Props file that checks the dependency: C:\Users\vmlab\source\repos\3D_TimeDivision_ParallaxBarrier_with_LenticularLens-master--tatebarrior\ProjectPB\RealSenseTracking.props

Done when:
- vcpkg is installed and `vcpkg list` shows `librealsense2` and `opencv4` for the chosen triplet, and an incremental build (cppupgrade_build_and_get_issues) no longer reports the original "Install vcpkg dependencies or set TrackingDepsDir" error.
