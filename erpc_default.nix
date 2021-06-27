with import <nixpkgs> {};
let 
    kernel = linuxPackages_4_14.kernel;
in stdenv.mkDerivation {
    name = "env";
    buildInputs = [
        bashInteractive
        numactl
        pkg-config
        boost
        linuxHeaders
        cmake
        gcc8
        valgrind
        protobuf
        clang-tools
        rdma-core
        gtest
        gflags
        gdb
	unzip
    ];
  cmakeFlags = [ "-DTRANSPORT=infiniband" "-DROCE=ON" "-DPERF=ON" ];
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
