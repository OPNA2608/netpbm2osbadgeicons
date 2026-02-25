let
  pins = import ./pins.nix;
in
{
  pkgs ? import (builtins.fetchTarball pins.nixpkgs) { },
}:

pkgs.mkShell {
  packages = with pkgs; [
    cmake
    editorconfig-checker
    llvmPackages_22.clang-tools
    netpbm
    nixfmt
    pre-commit
    python3Packages.identify
    valgrind
  ];

  shellHook = ''
    pre-commit install
  '';
}
