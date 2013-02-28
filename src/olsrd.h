#include <stdlib.h>
#include "process.h"
#include "util.h"

#define OLSR_HNA4 (1 << 0)
#define OLSR_HNA6 (1 << 1)
#define OLSR_IFACE_MESH (1 << 0)
#define OLSR_IFACE_ETHER (1 << 1)

typedef struct {
  process_t proto;
} olsrd_process_t;

typedef struct {
  char *key;
  char *value;
} olsrd_conf_item_t;

typedef struct {
  char *name; 
  olsrd_conf_item_t **attr;
  int num_attr;
} olsrd_conf_plugin_t;

typedef struct {
  char *ifname;
  int mode;
  char *Ipv4Broadcast;
} olsrd_conf_iface_t;

typedef struct {
  int family;
  char *address;
  char *netmask;
} olsrd_conf_hna_t;

int co_olsrd_add_iface(const char* name, int mode, const char *Ipv4Broadcast);

int co_olsrd_remove_iface(const char* name, const int mode, const char *Ipv4Broadcast);

int co_olsrd_add_hna(const int family, const char *address, const char *netmask);

int co_olsrd_remove_hna(const int family, const char *address, const char *netmask);

int co_olsrd_print_conf(const char *filename);

int olsrd_init(void *self);
