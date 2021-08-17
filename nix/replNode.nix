with import <nixpkgs> {};
with pkgs;
let
  my-python-packages = python-packages: with python-packages; [
    pandas
  ]; 
  python-with-my-packages = python3.withPackages my-python-packages;
in 
stdenv.mkDerivation {
    name = "replNode";
    buildInputs = [
	stdenv
	rdma-core
	numactl
	pmdk
	fio
    boost
    python-with-my-packages
    ];
  nativeBuildInputs = [
    cmake
    pkg-config
  ];
}
