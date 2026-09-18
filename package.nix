{
  lib,
  stdenv,
  runCommand,
  cmake,
  ctestCheckHook,
  gcovr,
  netpbm,
  valgrind,

  breakpointHook,
}:

let
  version = lib.strings.trim (lib.strings.readFile ./version);
in
stdenv.mkDerivation (finalAttrs: {
  pname = "netpbm2osbadgeicons";
  inherit version;

  src =
    let
      srcs = [
        "tests"
        "CMakeLists.txt"
        "version"
        "netpbm2osbadgeicons.c"
      ];
    in
    runCommand "netpbm2osbadgeicons-src" { } (
      ''
        mkdir $out
      ''
      + (lib.strings.concatMapStringsSep "\n" (
        filename: "ln -vs ${./. + "/${filename}"} $out/${filename}"
      ) srcs)
    );

  outputs = [ "out" ] ++ lib.optionals finalAttrs.doCheck [ "coverage" ];

  strictDeps = true;

  nativeBuildInputs = [
    cmake

    breakpointHook
  ];

  buildInputs = [
    netpbm
  ];

  nativeCheckInputs = [
    ctestCheckHook
    gcovr
    valgrind
  ];

  cmakeFlags = [
    (lib.strings.cmakeBool "NETPBM2OSBADGEICONS_WERROR" true)
    (lib.strings.cmakeBool "NETPBM2OSBADGEICONS_TEST_VALGRIND" true)
  ];

  postConfigure =
    let
      coverageFlags = [
        (lib.strings.cmakeFeature "CMAKE_BUILD_TYPE" "Coverage")
        (lib.strings.cmakeFeature "CMAKE_INSTALL_PREFIX" "/homeless-shelter")
      ];
      cmakeCoverageConfigureArgs = "-B ../build-coverage ${
        toString (coverageFlags ++ finalAttrs.cmakeFlags)
      } ..";
    in
    lib.optionalString finalAttrs.finalPackage.doCheck ''
      echo cmake ${cmakeCoverageConfigureArgs}
      cmake ${cmakeCoverageConfigureArgs}
    '';

  postBuild =
    let
      cmakeCoverageBuildArgs = "--build ../build-coverage \${enableParallelBuilding:+-j$NIX_BUILD_CORES} --verbose";
    in
    lib.optionalString finalAttrs.finalPackage.doCheck ''
      echo cmake ${cmakeCoverageBuildArgs}
      cmake ${cmakeCoverageBuildArgs}
    '';

  doCheck = stdenv.buildPlatform.canExecute stdenv.hostPlatform;

  postCheck =
    let
      ctestCoverageArgs = "--output-on-failure";
      gcovrArgs = "--html --html-details --html-single-page --root .. --output coverage.html";
    in
    lib.optionalString finalAttrs.finalPackage.doCheck ''
      pushd ../build-coverage
      echo ctest -j1 ${ctestCoverageArgs}
      ctest -j1 ${ctestCoverageArgs}

      echo gcovr ${gcovrArgs}
      gcovr ${gcovrArgs}
      popd
    '';

  postInstall = lib.optionalString finalAttrs.finalPackage.doCheck ''
    install -Dm644 ../build-coverage/coverage.html $coverage/coverage.html
  '';

  # For debugging:
  # cmakeBuildType = "Debug";
  # enableParallelChecking = false;
  # ctestFlags = [ "--verbose" ];
})
