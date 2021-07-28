# Replication Node
A shared log implementation. The Network stack is based on eRPC. 
As replication protocol a simple chain replication is used.
For logging the PMDK library has been extended and is used for logging on persistent memory.

## Building
There are nix-shell files in the nix folder, which are making the build process very easy.
In the specific files are all the dependencies listed which are needed to built the specific project.
Before using this code, the eRPC and PMDK project have to be built.
These git submodules have to be initialized and pulled.

### Transport Layer: DPDK
#### Building DPDK
1. Switch to the dpdk.nix shell.
```bash
nix-shell dpdk.nix
```
2. Change to the dpdk directory
```bash
cd dpdk
```
4. Execute make. The output directory is specified in the dpdk.nix as $makeFlags
```bash
make install $makeFlags

### Building eRPC-dpdk
1. Switch to the erpc-dpdk.nix shell.
```bash
nix-shell erpc-dpdk.nix
```
2. Change to the erpc-dpdk directory.
```bash
cd erpc-dpdk
```
3. Execute cmake with the cmakeFlags specified in the erpc-dpdk.nix file.
```bash
cmake $cmakeFlags
```
4. Execute make. 
```bash
make
```

### Transport Layer: RDMA
### Building eRPC
1. Switch to the erpc.nix shell.
```bash
nix-shell erpc.nix
```
2. Change to the erpc directory.
```bash
cd erpc
```
3. Execute cmake with the cmakeFlags specified in the erpc.nix file.
```bash
cmake $cmakeFlags
```
4. Execute make. 
```bash
make
```

### Storage Layer: PMDK
#### Building PMDK
1. Switch to the pmdk.nix shell.
```bash
nix-shell pmdk.nix
```
2. Change to the pmdk directory
```bash
cd pmdk
```
4. Execute make. The output directory is specified in the pmdk.nix as $makeFlags
```bash
make install $makeFlags
```

### Application
#### Building Replication Node
1. Change the paths in the CMakeLists.txt for dpdk/pmdk/erpc and other parameters needed.
2. Switch to the replNode.nix shell.
```bash
nix-shell replNode.nix
```
3. Execute cmake.
```bash
cmake .
```
4. Execute make.
```bash
make
```
