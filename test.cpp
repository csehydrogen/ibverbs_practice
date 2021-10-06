#include <cstdio>
#include <cstdlib>
#include <infiniband/verbs.h>

#include "util.h"

int main() {
  /*
   * 1. Get context (= open device)
   */
  int num_devices;
  ibv_device** device_list = ibv_get_device_list(&num_devices);
  CHECK_PTR(device_list);

  if (num_devices == 0) {
    printf("No RDMA devices found.\n");
    exit(EXIT_FAILURE);
  }

  printf("%d RDMA devices found:\n", num_devices);
  for (int i = 0; i < num_devices; ++i) {
    const char* name = ibv_get_device_name(device_list[i]);
    CHECK_PTR(name);
    printf("  [%d] %s\n", i, name);
  }

  // Use the first device
  ibv_context* context = ibv_open_device(device_list[0]);
  CHECK_PTR(context);

  // Be sure to free device list
  ibv_free_device_list(device_list);

  /*
   * 2. Create a PD
   */

  ibv_pd* pd = ibv_alloc_pd(context);
  CHECK_PTR(pd);

  /*
   * ?. Register MR
   */

  // Alloc 1GB
  uint32_t bufsz = 1 * 1024 * 1024 * 1024;
  void* buf = malloc(bufsz); 

  ibv_mr* mr = ibv_reg_mr(pd, buf, bufsz, IBV_ACCESS_LOCAL_WRITE);
  CHECK_PTR(mr);

  /*
   * ?. Create a completion event channel
   */

  ibv_comp_channel* channel = ibv_create_comp_channel(context);
  CHECK_PTR(channel);

  /*
   * ?. Create a CQ
   */

  const int CQE = 16; // CQ size
  ibv_cq* cq = ibv_create_cq(context, CQE, nullptr, channel, 0);
  CHECK_PTR(cq);

  /*
   * ?. Create a QP
   */

  ibv_qp_init_attr qp_init_attr;
  memset(&qp_init_attr, 0, sizeof(qp_init_attr));
  qp_init_attr.qp_context = nullptr;
  qp_init_attr.send_cq = cq;
  qp_init_attr.recv_cq = cq;
  qp_init_attr.srq = nullptr;
  qp_init_attr.cap.max_send_wr = 1; // max # of outstanding SR in SQ
  qp_init_attr.cap.max_recv_wr = 1; // max # of outstanding RR in RQ
  qp_init_attr.cap.max_send_sge = 1;
  qp_init_attr.cap.max_recv_sge = 1;
  qp_init_attr.cap.max_inline_data = 0;
  qp_init_attr.qp_type = IBV_QPT_RC;
  qp_init_attr.sq_sig_all = 0;

  ibv_qp* qp = ibv_create_qp(pd, &qp_init_attr);
  CHECK_PTR(qp);

  /*
   * ?. Post a SR
   */

  ibv_sge sge;
  memset(&sge, 0, sizeof(sge));
  sge.addr = (uint64_t)buf;
  sge.length = bufsz;
  sge.lkey = mr->lkey;

  ibv_send_wr send_wr;
  memset(&send_wr, 0, sizeof(send_wr));
  send_wr.next = nullptr;
  send_wr.sg_list = &sge;
  send_wr.num_sge = 1;
  send_wr.opcode = IBV_WR_SEND;

  ibv_send_wr *bad_wr;
  CHECK_INT(ibv_post_send(qp, &send_wr, &bad_wr));

  /*
   * ?. Poll a CQ
   */

  ibv_wc wc;
  int num_wc = ibv_poll_cq(cq, 1, &wc);
  printf("num_wc = %d\n", num_wc);
  printf("WC status: %s(%d)\n", ibv_wc_status_str(wc.status), wc.status);

  /*
   * Clean up
   */

  CHECK_INT(ibv_destroy_qp(qp));
  CHECK_INT(ibv_destroy_cq(cq));
  CHECK_INT(ibv_destroy_comp_channel(channel));
  CHECK_INT(ibv_dereg_mr(mr));
  CHECK_INT(ibv_dealloc_pd(pd));
  CHECK_INT(ibv_close_device(context));

  return 0;
}
