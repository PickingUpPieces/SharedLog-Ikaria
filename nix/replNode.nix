with import <nixpkgs> {};
stdenv.mkDerivation {
    name = "pmdk";
    buildInputs = [
	stdenv
	rdma-core
	numactl
	pmdk
	fio
    boost
    ];
  nativeBuildInputs = [
    cmake
    pkg-config
  ];
}
