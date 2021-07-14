with import <nixpkgs> {};
stdenv.mkDerivation {
    name = "pmdk";
    buildInputs = [
	stdenv
	rdma-core
	numactl
    pmdk
    ];
  nativeBuildInputs = [
    cmake
    pkg-config
  ];
}
