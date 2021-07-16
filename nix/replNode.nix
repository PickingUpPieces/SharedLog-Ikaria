with import <nixpkgs> {};
stdenv.mkDerivation {
    name = "pmdk";
    buildInputs = [
	stdenv
	rdma-core
	numactl
	pmdk
	fio
    ];
  nativeBuildInputs = [
    cmake
    pkg-config
  ];
}
