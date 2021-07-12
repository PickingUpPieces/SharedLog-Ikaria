with import <nixpkgs> {};
stdenv.mkDerivation {
    name = "pmdk";
    buildInputs = [
	stdenv
	rdma-core
	numactl
    ];
  nativeBuildInputs = [
    cmake
    pkg-config
  ];
}
