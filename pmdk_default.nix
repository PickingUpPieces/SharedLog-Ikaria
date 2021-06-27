with import <nixpkgs> {};
stdenv.mkDerivation {
    name = "env";
    buildInputs = [
        bashInteractive
        numactl
        pkg-config
        boost
        linuxHeaders
        gcc8
        valgrind
        vimPlugins.vim-clang-format
        clang-tools
        gtest
        gflags
        gdb
	unzip
	libndctl
	gnum4
	pandoc
    ];
  makeFlags = ["DESTDIR=/home/vincent/ba-single-node/pmdk/lib" "prefix="];
  NIX_CFLAGS_COMPILE = [
    # nobody got time to fix all these errors!
    "-Wno-error"
  ];
  nativeBuildInputs = [
    cmake
    pkg-config
  ];
}
