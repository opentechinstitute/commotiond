#include <stdlib.h>
#include "extern/list.h"
#include "debug.h"
#include "util.h"
#include "olsrd.h"

process_t olsrd_process_proto = {
  .init = olsrd_init
};

static olsrd_conf_item_t global_items[];

static olsrd_conf_plugin_t plugins[];

static list_t *ifaces;

static list_t *hna;

static void _co_olsrd_print_iface_i(list_t *list, lnode_t *lnode, void *context) {
  olsrd_conf_face_t *iface = lnode_get(lnode);

  //Print to file

  return;
}

static void _co_olsrd_print_hna_i(list_t *list, lnode_t *lnode, void *context) {
  olsrd_conf_face_t *iface = lnode_get(lnode);

  //Print to file

  return;
}

int co_olsrd_add_iface(const char* name, int mode, const char *Ipv4Broadcast, const char *Ipv6Broadcast) {
  return 1;
}
int co_olsrd_remove_iface(const char* name, const int mode, const char *Ipv4Broadcast, const char *Ipv6Broadcast) {
  return 1;
}

int co_olsrd_add_hna(const int family, const char *address, const char *netmask) {
  return 1;
error:
  return 0;
}

int co_olsrd_remove_hna(const int family, const char *address, const char *netmask) {
  return 1;
error:
  return 0;
}

int co_olsrd_print_conf(const char *filename) {
  return 1;
error:
  return 0;
}

int olsrd_init(void *self) {
  olsrd_process_t *this = self;
  //This function gets called when the process object is created, and should call any initialization stuff that happens before it starts
}
