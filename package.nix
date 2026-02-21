{
  lib,
  stdenv,
  runCommand,
  cmake,
  ctestCheckHook,
  netpbm,
  valgrind,
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "netpbm2osbadgeicons";
  version = lib.strings.trim (lib.strings.readFile ./version);

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

  strictDeps = true;

  nativeBuildInputs = [
    cmake
  ];

  buildInputs = [
    netpbm
  ];

  nativeCheckInputs = [
    ctestCheckHook
    valgrind
  ];

  cmakeFlags = [
    (lib.strings.cmakeBool "NETPBM2OSBADGEICONS_WERROR" true)
    (lib.strings.cmakeBool "NETPBM2OSBADGEICONS_TEST_VALGRIND" true)
  ];

  doCheck = stdenv.buildPlatform.canExecute stdenv.hostPlatform;

  # For debugging:
  # cmakeBuildType = "Debug";
  # enableParallelChecking = false;
  # ctestFlags = [ "--verbose" ];
})
