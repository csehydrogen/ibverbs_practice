#include <cstdio>
#include <unistd.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>

#include "util.h"

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("Usage: %s [port]\n", argv[0]);
    return 0;
  }
  char* port = argv[1];

  /*
   * Setup address info. For server,
   * 1. Set hint.ai_flags to RAI_PASSIVE
   * 2. Pass NULL to name
   */
  rdma_addrinfo hint;
  memset(&hint, 0, sizeof(hint));
  hint.ai_flags = RAI_PASSIVE;
  hint.ai_port_space = RDMA_PS_TCP;
  rdma_addrinfo* addrinfo;
  CHECK_INT(rdma_getaddrinfo(NULL, port, &hint, &addrinfo));

  /*
   * Create CM ID for listening.
   * QP attributes are saved and will be used for each accepted CM ID.
   */
  ibv_qp_init_attr qp_init_attr;
  memset(&qp_init_attr, 0, sizeof(qp_init_attr));
  qp_init_attr.cap.max_send_wr = 1;
  qp_init_attr.cap.max_recv_wr = 1;
  qp_init_attr.cap.max_send_sge = 1;
  qp_init_attr.cap.max_recv_sge = 1;
  qp_init_attr.sq_sig_all = 1;
  rdma_cm_id* listen_id;
  CHECK_INT(rdma_create_ep(&listen_id, addrinfo, NULL, &qp_init_attr));
  rdma_freeaddrinfo(addrinfo);

  /*
   * Listen for connections.
   */
  const int BACKLOG = 8;
  CHECK_INT(rdma_listen(listen_id, BACKLOG));

  /*
   * Get the next pending connection request event (blocking)
   */
  rdma_cm_id* id;
  CHECK_INT(rdma_get_request(listen_id, &id));

  /*
   * Prepare MR for send/recv messages
   */
  size_t buf_sz = 1L * 1024 * 1024 * 1024;
  void* buf = malloc(buf_sz);
  ibv_mr* mr = rdma_reg_msgs(id, buf, buf_sz);
  CHECK_PTR(mr);

  /*
   * Test
   */
  ibv_wc wc;
  double st, bw;
  int num_wc;

  CHECK_INT(rdma_post_recv(id, NULL, buf, buf_sz, mr));
  CHECK_INT(rdma_accept(id, NULL));
  for (int i = 0; ; ++i) {
    st = GetTime();
    num_wc = rdma_get_recv_comp(id, &wc);
    assert(num_wc == 1);
    CHECK_WC_STATUS(wc.status);
    CHECK_INT(rdma_post_recv(id, NULL, buf, buf_sz, mr));
    CHECK_INT(rdma_post_send(id, NULL, buf, buf_sz, mr, 0));
    num_wc = rdma_get_send_comp(id, &wc);
    assert(num_wc == 1);
    CHECK_WC_STATUS(wc.status);
    bw = (2 * buf_sz) / (GetTime() - st) / 1e9;
    printf("[%d] Bandwidth: %f GB/s\n", i, bw);
  }

  return 0;
}
