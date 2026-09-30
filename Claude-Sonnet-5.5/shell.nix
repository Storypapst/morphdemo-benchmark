# Build-time tools for the MorphDemo executables (runtime uses only libraries already on the host).
{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
  packages = [ pkgs.nasm ];
}
