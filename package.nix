{
  lib,
  stdenv,
  runCommand,
  cmake,
  ctestCheckHook,
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

  nativeCheckInputs = [
    ctestCheckHook
  ];

  cmakeFlags = [
    (lib.strings.cmakeBool "NETPBM2OSBADGEICONS_WERROR" true)
  ];

  doCheck = stdenv.buildPlatform.canExecute stdenv.hostPlatform;

  # For debugging:
  # cmakeBuildType = "Debug";
  # enableParallelChecking = false;
  # ctestFlags = [ "--verbose" ];
})
