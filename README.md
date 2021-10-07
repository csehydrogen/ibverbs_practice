# ibverbs_practice

## Study

### Channel Adapter (CA)

* End node in subnet. (= InfiniBand Card)
* HCA = CA which supports verbs interface, TCA = weak version of CA
* Each cable hole is called "Port".

### Network Layer

* InfiniBand supports up to transport layer.
* Each subnet should be managed by a subnet manager.
* If a switch has a subnet manager(= managed switch), Good. If not, install OpenSM on one of hosts.

### Terms

* Local Identifier(LID): Unique value in a subnet. Each port has one or range of LIDs. All switch ports have the same LID
* Work Request(WR): Work items that the HW should perform
* Work Queue(WQ): A queue with Work Queue Entries(WQEs). Enqueued by app, Dequeued by hardware.
    * Enqueue is called "Post"
* Send Queue(SQ): WQ for sending messages. Each entry is called a Send Request(SR).
    * Send != Write to remote. It can be a read request.
* Receive Queue(RQ): WQ for receiving messages. Each entry is called a Receive Request(RR).
* Queue Pair(QP): SQ + RQ. Associated with a partition key(P_Key).
* Completion Queue(CQ): A queue with completed WR information(Work Completion, WC). Enqueued by hardware, Dequeued by app.
    * CQ's size is fixed. The user should allocate enough size, and dequeue the CQ to prevent overflow.
    * CQ can be assigned to QPs, SQs, RQs.
* Requestor: Active side. Initiates data transfer.
* Responder: Passive side. Send or receive data, depending on opcode.
* Context: Literally, context associated with a device. You can get one context for opening one device.
* Protection Domain(PD): Associates QP with other resources such as memory regions.
    * Resources with the different PDs cannot work together
* Memory Region(MR): A virtually contiguous memory block, registered for work with RDMA.
    * Associated with Local Key(lkey) and Remote Key(rkey)
* Completion Event Channel: a mechanism for delivering notification about the creation of work completions in CQs
    * Can attach multiple CQs
* End to End(EE): ???
* Scatter Gather Entry(SGE): Each WR usually contains >=1 SGEs. No SGE means 0-byte message.
    * Gather: when local data is read and sent over the wire
    * Scatter: when data is received and written locally
    * Basically a view from CA to memory

### ibverbs

* Verbs is a low-level description (not implementation) for RDMA programming
* libibverbs is de-facto API standard for verbs
    * Same API for all RDMA-enable transport protocols
    * InfiniBand: InfiniBand NIC + InfiniBand switch
    * RoCE: RoCE-supported NIC + standard Ethernet switch
    * iWARP: iWARP-supported NIC + standard Ethernet switch
* Completely thread-safe
    * When possible, avoid using fork()

### Implementation

* Header: `#include <infiniband/verbs.h>`
* Link: `-libverbs`
* Initialize all input structures to zero

### Program Flow

1. Get device lists, pick a device, and open a context associated with the device.
2. Create a PD.
3. Create a MR.
4. Create a completion event channel
5. Create a CQ
6. Create a QP
    * How to connect QP X with QP Y?: out of band(e.g., over socket), communication manager(CM)
    * CM is the right way
7. FORGET IT. I DECIDED TO USE RDMACM.

### rdmacm

rdmacm is a slightly higher-level API than ibverbs.

* `rdma_getaddrinfo`: Construct `rdma_addrinfo` with address and port. Address can be ip address or even hostname.
    * For server, give it no address (it will listen on 0.0.0.0) and `RAI_PASSIVE` flag.
* `rdma_create_ep`: Construct CM ID with QP attributes
    * For client, new QP is made and associated with the CM ID
    * For server, the CM ID is for listen, not actual connection. QP attributes are saved and will be used for CM IDs accepted in the future.
* (Server only) `rdma_listen` and `rdma_get_request`
* `rdma_reg_msgs`: Create MRs
* Send/recv messages
    * Tricky part is, the remote peer should queue a RR BEFORE the local issues send operations. (Check `rdma_post_send` documentation.)
    * In pingpong app, for example, `rdma_post_recv` should be called before `rdma_post_send`.
    * Server: `rdma_post_recv` - `rdma_accept` - (`rdma_get_recv_comp` - `rdma_post_recv` - `rdma_post_send` - `rdma_get_send_comp`)+
    * Client: `rdma_connect` - (`rdma_post_recv` - `rdma_post_send` - `rdma_get_send_comp` - `rdma_get_recv_comp`)+

### Scenario

IB programming is already hard. What makes it harder is that there are multiple ways to do the same job. I've seen several flows using those APIs.

1. Like the previous section, obtain a connection (QP) using `rdma_getaddrinfo` and `rdma_create_ep`, and use RDMA Verbs APIs with `rdma_cm_id` to communicate.
2. Utilize RDMA CM APIs to bind to NICs by address(`rdma_create_id` and `rdma_bind_addr` along with `getaddrinfo`). Now, `rdma_cm_id::verbs` contains valid `ibv_context`. Use the context to do your jobs with VPI Verbs APIs.
3. Use `ibv_get_device_list` and `ibv_open_device` to get `ibv_context`. Use VPI Verbs APIs.
4. There are more...

## Running Tests

```bash
$ make
$ ./server [port] # server
$ ./client [ip] [port] # client
```

IP should be infiniband-assigned one, not ethernet.

## References

* https://www.mellanox.com/related-docs/prod_software/RDMA_Aware_Programming_user_manual.pdf : Reference for Mellanox implementation
* https://www.csm.ornl.gov/workshops/openshmem2014/documents/presentations_and_tutorials/Tutorials/Verbs%20programming%20tutorial-final.pdf : Great workshop-style tutorial
* https://insujang.github.io/2020-02-09/introduction-to-programming-infiniband/ : Good summary by a student from KAIST
