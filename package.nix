{
  lib,
  stdenv,
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "netpbm2osbadgeicons";
  version = lib.strings.trim (lib.strings.readFile ./version);

  src = ./netpbm2osbadgeicons.c;
  dontUnpack = true;

  buildPhase = ''
    runHook preBuild

    $CC -std=c99 -Wall -Wextra -pedantic -Werror $src -o netpbm2osbadgeicons

    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall

    install -Dm755 netpbm2osbadgeicons -t $out/bin

    runHook postInstall
  '';
})
