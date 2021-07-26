with import <nixpkgs> {};
let 
    kernel = linuxPackages_4_14.kernel;
in stdenv.mkDerivation {
    name = "env";
    buildInputs = [
        numactl
        pkg-config
        cmake
        gcc8
        protobuf
        rdma-core
	gtest
	gflags
    ];
  cmakeFlags = [ "-DTRANSPORT=infiniband" "-DROCE=ON" "-DPERF=ON" "-DLOG_LEVEL=none" ];
  NIX_CFLAGS_COMPILE = [
    # nobody got time to fix all these errors!
    "-Wno-error"
  ];
  nativeBuildInputs = [
    cmake
    pkg-config
  ];
NIXOS_KERNELDIR = "${kernel.dev}/lib/modules/${kernel.modDirVersion}/build";
}
