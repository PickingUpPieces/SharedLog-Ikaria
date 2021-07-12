# Replication Node
A shared log implementation. The Network stack is based on eRPC. 
As replication protocol a simple chain replication is used.
For logging the PMDK library has been extended and is used for logging on persistent memory.

## Building
There are nix-shell files in the nix folder, which are making the build process very easy.
In the specific files are all the dependencies listed which are needed to built the specific project.
Before using this code, the eRPC and PMDK project have to be built.
These git submodules have to be initialized and pulled.

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

### Building PMDK
1. Switch to the pmdk.nix shell.
```bash
nix-shell pmdk.nix
```
2. Change to the erpc directory
```bash
cd pmdk
```
4. Execute make. The output directory is specified in the erpc.nic as $makeFlags
```bash
make install $makeFlags
```

### Building Replication Node
1. Change the paths in the CMakeLists.txt for pmdk/erpc and other parameters needed.
2. Switch to the repl_node.nix shell.
```bash
nix-shell repl_node.nix
```
3. Execute cmake.
```bash
cmake
```
4. Execute make.
```bash
make
```