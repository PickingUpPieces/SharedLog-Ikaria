# A shared log implementation with persistent memory and eRPC
Shared logs are one of the basic building blocks of distributed systems. They are used
in system like Kafka, Amazon Aurora or LogDevice. The increased popularity
of shared logs in the last decade is due to their offered simplicity and properties
like Strong Consistency. They order events received from multiple clients, replicate
them across multiple servers and make them accessible. Therefore, a shared log can
drastically decrease complexity for the system built on top. As it is a core building
block, the shared log needs to be as performant as possible. In recent years there have
been advancements in the fields of networking, Persistent Memory, and multi-core
processors. The open question is how these advancements can be leveraged in a
shared log system. We designed and implemented Ikaria, a highly parallelized shared
log with CRAQ as a replication algorithm. Ikaria makes use of asynchronous user-
space networking and builds upon Persistent Memory, which offers byte-addressable
access, persistent data storage, and performance close to volatile memory. Our results
show that Ikaria offers high throughput for read-heavy workloads. Furthermore, we
demonstrate that Ikaria scales well with an increasing number of servers, threads, and
different log entry sizes compared to the original Chain Replication protocol.

For more details, refer to the [thesis](https://github.com/TUM-DSE/research-work-archive/blob/main/archive/2021/winter/docs/bsc_picking_shared_log_with_persistent_memory_and_erpc.pdf).


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
#### Configure NIC for DPDK
1. Build eRPC library along with the DPDK (I used v19.08-rc2 and it worked with minimal changes on the DPDK side. NOTE: eRPC is not supported for the latest DPDK versions) 
2. ```cd DPDK_PATH/usertools``` -- there are a bunch of scripts there. We are interested in the dpdk-devbind.py. 
3. ```Execute python dpdk-devbind.py --status``` will give you the information about the available interfaces, etc. 
For example:  
```
Network devices using kernel driver 
=================================== 
0000:00:1f.6 'Ethernet Connection (7) I219-V 15bc' if=eno1 drv=e1000e unused=igb_uio *Active* 
0000:02:00.0 'Ethernet Controller XL710 for 40GbE QSFP+ 1583' if=enp2s0f0 drv=i40e unused=igb_uio  
0000:02:00.1 'Ethernet Controller XL710 for 40GbE QSFP+ 1583' if=enp2s0f1 drv=i40e unused=igb_uio *Active* 
```
4. Among the two interfaces (enp2s0f0 and enp2s0f1) you need to find which one is UP. You can use `ip addr show`. 
```
   2: enp2s0f0: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc mq state DOWN group default qlen 1000 
   4: enp2s0f1: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc mq state UP group default qlen 1000 
```

6. You need to bind the interface to a user space driver so the kernel will not see it. You do this by `sudo python dpdk-devbind.py --bind=igb_uio <INTERFACE>`. If the network is active as in our case that means that there is a route table for that interface so simply use `sudo python dpdk-devbind.py --force --bind=igb_uio <INTERFACE>`.
7. The status is now: 
```
   Network devices using DPDK-compatible driver 
============================================ 
0000:02:00.1 'Ethernet Controller XL710 for 40GbE QSFP+ 1583' drv=igb_uio unused=i40e 

   Network devices using kernel driver 
=================================== 
0000:00:1f.6 'Ethernet Connection (7) I219-V 15bc' if=eno1 drv=e1000e unused=igb_uio *Active* 
0000:02:00.0 'Ethernet Controller XL710 for 40GbE QSFP+ 1583' if=enp2s0f0 drv=i40e unused=igb_uio 
```
8. To bind it back to the kernel driver please use the ```sudo  python dpdk-devbind.py --force --bind=i40e 0000:02:00.1```. You have to use the PCI address since the convention of the interface name is not visible (not bound in the kernel).



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
