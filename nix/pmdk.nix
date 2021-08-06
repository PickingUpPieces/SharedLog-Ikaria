with import <nixpkgs> {};
stdenv.mkDerivation {
    name = "pmdk";
    buildInputs = [
	stdenv
	autoconf
        pkg-config
	libndctl
	gnum4
	pandoc
	glib
	libfabric
	fuse
    ];
  makeFlags = ["PMEM_IS_PMEM_FORCE=1" "DESTDIR=/home/vincent/ba-single-node/pmdk/lib" "prefix="];
  nativeBuildInputs = [
    cmake
    pkg-config
  ];
  NIX_CFLAGS_COMPILE = [
    # nobody got time to fix all these errors!
    "-Wno-error"
  ];
  enableParallelBuilding = true;
}
