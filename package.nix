{
  lib,
  stdenv,
  runCommand,
  cmake,
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "netpbm2osbadgeicons";
  version = lib.strings.trim (lib.strings.readFile ./version);

  src =
    let
      srcs = [
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

  cmakeFlags = [
    (lib.strings.cmakeBool "NETPBM2OSBADGEICONS_WERROR" true)
  ];
})
