#include <cstdio>
#include <unistd.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>

#include "util.h"

int main(int argc, char** argv) {
  if (argc != 3) {
    printf("Usage: %s [addr] [port]\n", argv[0]);
    return 0;
  }
  char* addr = argv[1];
  char* port = argv[2];

  /*
   * Setup address info.
   */
  rdma_addrinfo hint;
  memset(&hint, 0, sizeof(hint));
  hint.ai_port_space = RDMA_PS_TCP;
  rdma_addrinfo* addrinfo;
  CHECK_INT(rdma_getaddrinfo(addr, port, &hint, &addrinfo));

  /*
   * Create CM ID with QP attributes.
   */
  ibv_qp_init_attr qp_init_attr;
  memset(&qp_init_attr, 0, sizeof(qp_init_attr));
  qp_init_attr.cap.max_send_wr = 1;
  qp_init_attr.cap.max_recv_wr = 1;
  qp_init_attr.cap.max_send_sge = 1;
  qp_init_attr.cap.max_recv_sge = 1;
  qp_init_attr.sq_sig_all = 1;
  rdma_cm_id* id;
  CHECK_INT(rdma_create_ep(&id, addrinfo, NULL, &qp_init_attr));
  rdma_freeaddrinfo(addrinfo);

  /*
   * Prepare MRs for send/recv messages
   */
  size_t buf_sz = 1L * 1024 * 1024 * 1024;

  void* buf_send = malloc(buf_sz);
  ibv_mr* mr_send = rdma_reg_msgs(id, buf_send, buf_sz);
  CHECK_PTR(mr_send);

  void* buf_recv = malloc(buf_sz);
  ibv_mr* mr_recv = rdma_reg_msgs(id, buf_recv, buf_sz);
  CHECK_PTR(mr_recv);

  /*
   * Test
   */
  ibv_wc wc;
  double st, bw;
  int num_wc;

  CHECK_INT(rdma_connect(id, NULL));
  for (int i = 0; ; ++i) {
    st = GetTime();
    CHECK_INT(rdma_post_recv(id, NULL, buf_recv, buf_sz, mr_recv));
    CHECK_INT(rdma_post_send(id, NULL, buf_send, buf_sz, mr_send, 0));
    num_wc = rdma_get_send_comp(id, &wc);
    assert(num_wc == 1);
    CHECK_WC_STATUS(wc.status);
    num_wc = rdma_get_recv_comp(id, &wc);
    assert(num_wc == 1);
    CHECK_WC_STATUS(wc.status);
    bw = (2 * buf_sz) / (GetTime() - st) / 1e9;
    printf("[%d] Bandwidth: %f GB/s\n", i, bw);
  }

  return 0;
}
