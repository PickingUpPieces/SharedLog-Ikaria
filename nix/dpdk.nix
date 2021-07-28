with import <nixpkgs> {};
stdenv.mkDerivation {
  name = "dpdk";
  nativeBuildInputs = [
    doxygen
    meson
    ninja
    pkg-config
    cmake
    python3
    python3.pkgs.sphinx
    python3.pkgs.pyelftools
  ];
  buildInputs = [
    jansson
    libbpf
    libbsd
    libelf
    libpcap
    numactl
    openssl.dev
    zlib
  ]; 
  makeFlags = ["T=x86_64-native-linuxapp-gcc" "DESTDIR=."];
  NIX_CFLAGS_COMPILE = [
    "-Wno-error"
    "-Wno-stringop-overflow"
    "-Wno-zero-length-bounds" 
    "-fcommon"
  ];
  enableParallelBuilding = true;
}
