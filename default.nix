let
  pins = import ./pins.nix;
in
{
  pkgs ? import (builtins.fetchTarball pins.nixpkgs) { },
}:

pkgs.callPackage ./package.nix { }
