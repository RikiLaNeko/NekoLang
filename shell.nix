{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = [
    pkgs.cmake
    pkgs.gcc
    pkgs.glew
    pkgs.glm
    pkgs.glfw
    pkgs.pkg-config
    pkgs.libGL
  ];

  shellHook = ''
    echo "Environnement de développement pour NekoLang prêt !"
  '';
}

