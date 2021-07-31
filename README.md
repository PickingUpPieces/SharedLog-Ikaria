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
```
<details>
<summary>Configure NIC for DPDK</summary>
1. Build eRPC library along with the DPDK (I used v19.08-rc2 and it worked with minimal changes on the DPDK side. NOTE: eRPC is not supported for the latest DPDK versions) <br>
2. cd DPDK_PATH/usertools -- there are a bunch of scripts there. We are interested in the dpdk-devbind.py. <br>
3. python dpdk-devbind.py --status will give you the information about the available interfaces, etc. <br>
   For example, in clara:  <br>
   Network devices using kernel driver <br>
=================================== <br>
0000:00:1f.6 'Ethernet Connection (7) I219-V 15bc' if=eno1 drv=e1000e unused=igb_uio *Active* <br>
0000:02:00.0 'Ethernet Controller XL710 for 40GbE QSFP+ 1583' if=enp2s0f0 drv=i40e unused=igb_uio  <br>
0000:02:00.1 'Ethernet Controller XL710 for 40GbE QSFP+ 1583' if=enp2s0f1 drv=i40e unused=igb_uio *Active* <br>
4. You always change the 40GbE's NIC since the first one is the university NIC and we *should* not touch it.<br>
5. Among the two interfaces (enp2s0f0 and enp2s0f1) you need to find which one is UP. You can use `ip addr show`. The output on clara is:<br>
   2: enp2s0f0: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc mq state DOWN group default qlen 1000 <br>
   4: enp2s0f1: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc mq state UP group default qlen 1000 <br>
   So interface enp2s0f1 is UP. Normally the the interfaces ending with 1 would be the connected ones. <br>
6. You need to bind the interface to a user space driver so the kernel will not see it. You do this by `sudo python dpdk-devbind.py --bind=igb_uio <INTERFACE>`. If the network is active as in our case that means that there is a route table for that interface so simply use `sudo python dpdk-devbind.py --force --bind=igb_uio <INTERFACE>`.<br>
7. The status is now: <br>
   Network devices using DPDK-compatible driver <br>
============================================ <br>
0000:02:00.1 'Ethernet Controller XL710 for 40GbE QSFP+ 1583' drv=igb_uio unused=i40e <br>
   Network devices using kernel driver <br>
=================================== <br>
0000:00:1f.6 'Ethernet Connection (7) I219-V 15bc' if=eno1 drv=e1000e unused=igb_uio *Active* <br>
0000:02:00.0 'Ethernet Controller XL710 for 40GbE QSFP+ 1583' if=enp2s0f0 drv=i40e unused=igb_uio <br>
8. To bind it back to the kernel driver please use the " sudo  python dpdk-devbind.py --force --bind=i40e 0000:02:00.1 ". You have to use the PCI address since the convention of the interface name is not visible (not bound in the kernel).<br>
</details>
<br>


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
