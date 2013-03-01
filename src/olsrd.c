/* vim: set ts=2 expandtab: */
#include <stdlib.h>
#include "extern/list.h"
#include "debug.h"
#include "util.h"
#include "olsrd.h"

process_t olsrd_process_proto = {
  .init = olsrd_init
};

static olsrd_conf_item_t *global_items;
static int global_items_count;

static olsrd_conf_plugin_t *plugins;
static int plugins_count;

static list_t *ifaces;

static list_t *hna;

static void _co_olsrd_print_iface_i(list_t *list, lnode_t *lnode, void *context) {
  olsrd_conf_iface_t *iface = lnode_get(lnode);
  FILE *conf_file = (FILE*)context;

  fprintf(conf_file, "interface\t%s\n", iface->ifname);
  fprintf(conf_file, "{\n");

  if (iface->mode & OLSR_IFACE_MESH)
    fprintf(conf_file, "\tMode\t\"mesh\"\n");
  else if (iface->mode & OLSR_IFACE_ETHER) 
    fprintf(conf_file, "\tMode\t\"ether\"\n");

  fprintf(conf_file, "\tIPv4Broadcast\t%s\n", iface->Ipv4Broadcast);

  fprintf(conf_file, "}\n");

  return;
}

static void _co_olsrd_print_hna_i(list_t *list, lnode_t *lnode, void *context) {
  olsrd_conf_hna_t *hna = lnode_get(lnode);
  FILE *conf_file = (FILE*)context;

  if (hna->family == 0)
    return;

  if (hna->family & OLSR_HNA4)
    fprintf(conf_file, "Hna4\n");
  else if (hna->family & OLSR_HNA6)
    fprintf(conf_file, "Hna6\n");

  fprintf(conf_file, "{\n");  

  fprintf(conf_file, "\t%s %s\n", hna->address, hna->netmask);

  fprintf(conf_file, "}\n");  

  return;
}

int co_olsrd_print_conf(const char *filename) {
  int i = 0;
  FILE *conf_file;

  if (!(conf_file = fopen(filename, "w+"))) {
    /* file could not be opened!
     */
    return 0;
  }

  for (i = 0; i<global_items_count; i++) {
    fprintf(conf_file, "%s\t%s\n", global_items[i].key, 
                                   global_items[i].value);
  }

  for (i = 0; i<plugins_count; i++) {
    int j = 0;
    fprintf(conf_file, "LoadPlugin\t\"%s\"\n", plugins[i].name);
    fprintf(conf_file, "{\n");
    for (j = 0; j<plugins[i].num_attr; j++) {
      fprintf(conf_file, "\t%s\t%s\n",plugins[i].attr[i]->key, 
                                      plugins[i].attr[i]->value);
    }
    fprintf(conf_file, "}\n");
  }

  list_process(ifaces, (void*)conf_file, _co_olsrd_print_iface_i);
  list_process(hna, (void*)conf_file, _co_olsrd_print_hna_i);

  fclose(conf_file);

  return 1;
}

static int _co_olsrd_compare_iface_i(const void *a, const void *b) {
  olsrd_conf_iface_t *iface_a = (olsrd_conf_iface_t*)a;
  olsrd_conf_iface_t *iface_b = (olsrd_conf_iface_t*)b;

  if (!strcmp(iface_a->ifname, iface_b->ifname) &&
      !strcmp(iface_a->Ipv4Broadcast, iface_b->Ipv4Broadcast) &&
      iface_a->mode == iface_b->mode)
    return 1;
  return 0;
}

static int _co_olsrd_compare_hna_i(const void *a, const void *b) {
  olsrd_conf_hna_t *hna_a = (olsrd_conf_hna_t*)a;
  olsrd_conf_hna_t *hna_b = (olsrd_conf_hna_t*)b;

  if (!strcmp(hna_a->address, hna_b->address) &&
      !strcmp(hna_a->netmask, hna_b->netmask) &&
      hna_a->family == hna_b->family)
    return 1;
  return 0;
}

int co_olsrd_add_iface(const char* name, int mode, const char *Ipv4Broadcast) {
  olsrd_conf_iface_t *new_iface = NULL;
  new_iface = (olsrd_conf_iface_t*)calloc(1, sizeof(olsrd_conf_iface_t));

  new_iface->mode = mode;

  new_iface->ifname = (char*)calloc(strlen(name)+1, sizeof(char));
  strcpy(new_iface->ifname, name);

  new_iface->Ipv4Broadcast=(char*)calloc(strlen(Ipv4Broadcast)+1,sizeof(char));
  strcpy(new_iface->Ipv4Broadcast, Ipv4Broadcast);

  list_append(ifaces, lnode_create(new_iface));
  return 1;
}
int co_olsrd_remove_iface(const char* name, const int mode, const char *Ipv4Broadcast) {
  olsrd_conf_iface_t *iface_to_remove;
  olsrd_conf_iface_t iface_to_find;
  lnode_t *iface_node_to_remove;

  /*
   * FYI: This causes a compiler warning for
   * discarding the constant. Using a strcpy()
   * would fix this, but that wastes time.
   */
  iface_to_find.mode = mode;
  iface_to_find.ifname = name;
  iface_to_find.Ipv4Broadcast = Ipv4Broadcast;

  if ((iface_node_to_remove = list_find(ifaces, 
                                       &iface_to_find, 
                                       _co_olsrd_compare_iface_i))) {
    /* 
     * delete from the list.
     */
    list_delete(ifaces, iface_node_to_remove);

    /*
     * free the object's memory.
     */
    iface_to_remove = lnode_get(iface_node_to_remove);
    free(iface_to_remove->ifname);
    free(iface_to_remove->Ipv4Broadcast);
    free(iface_to_remove);
  }
  return 1;
}

int co_olsrd_add_hna(const int family, const char *address, const char *netmask) {
  olsrd_conf_hna_t *new_hna = NULL;
  new_hna = (olsrd_conf_hna_t*)calloc(1, sizeof(olsrd_conf_hna_t));

  new_hna->family = family;

  new_hna->address = (char*)calloc(strlen(address)+1, sizeof(char));
  strcpy(new_hna->address, address);
  
  new_hna->netmask = (char*)calloc(strlen(netmask)+1, sizeof(char));
  strcpy(new_hna->netmask, netmask);

  list_append(hna, lnode_create(new_hna));
  return 1;
}

int co_olsrd_remove_hna(const int family, const char *address, const char *netmask) {
  olsrd_conf_hna_t *hna_to_remove;
  olsrd_conf_hna_t hna_to_find;
  lnode_t *hna_node_to_remove;

  /*
   * FYI: This causes a compiler warning for
   * discarding the constant. Using a strcpy()
   * would fix this, but that wastes time.
   */
  hna_to_find.family = family;
  hna_to_find.address = address;
  hna_to_find.netmask = netmask;

  if ((hna_node_to_remove = list_find(hna, 
                                     &hna_to_find, 
                                     _co_olsrd_compare_hna_i))) {
    /* 
     * delete from the list.
     */
    list_delete(hna, hna_node_to_remove);

    /*
     * free the object's memory.
     */
    hna_to_remove = lnode_get(hna_node_to_remove);
    free(hna_to_remove->address);
    free(hna_to_remove->netmask);
    free(hna_to_remove);
  }
  return 1;
}


int olsrd_init(void *self) {
  olsrd_process_t *this = self;
  //This function gets called when the process object is created, and should call any initialization stuff that happens before it starts
}

#if 0
int test_co_olsrd_print_conf(const char *filename) {
  co_olsrd_print_conf(filename);
}

int main() {
  olsrd_conf_hna_t test_hna, test_hna2;

  ifaces = list_create(5);
  hna = list_create(5);

  global_items = (olsrd_conf_item_t*)calloc(3, sizeof(olsrd_conf_item_t));
  global_items[0].key = "one";
  global_items[0].value = "1";
  global_items[1].key = "two";
  global_items[1].value = "2";
  global_items[2].key = "three";
  global_items[2].value = "3";
  global_items_count = 3;

  plugins = (olsrd_conf_plugin_t*)calloc(2, sizeof(olsrd_conf_plugin_t));
  plugins[0].attr = (olsrd_conf_item_t**)calloc(2, sizeof(olsrd_conf_item_t*));
  plugins[0].attr[0] = (olsrd_conf_item_t*)malloc(sizeof(olsrd_conf_item_t));
  plugins[0].attr[0]->key = "attr1";
  plugins[0].attr[0]->value = "value of attr1";
  plugins[0].attr[1] = (olsrd_conf_item_t*)malloc(sizeof(olsrd_conf_item_t));
  plugins[0].attr[1]->key = "attr2";
  plugins[0].attr[1]->value = "value of attr2";
  plugins[0].num_attr = 2;
  plugins[0].name = "Plugin1";

  plugins[1].attr = (olsrd_conf_item_t**)calloc(2, sizeof(olsrd_conf_item_t*));
  plugins[1].attr[0] = (olsrd_conf_item_t*)malloc(sizeof(olsrd_conf_item_t));
  plugins[1].num_attr = 0;
  plugins[1].name = "Plugin2";
  plugins_count = 2;

  /* 
   * Add two intefaces and two HNAs
   */
  co_olsrd_add_iface("eth0", OLSR_IFACE_MESH, "255.255.255.0");
  co_olsrd_add_iface("eth2", OLSR_IFACE_ETHER, "255.255.2.0");
  co_olsrd_add_hna(OLSR_HNA4, "192.168.1.1", "255.255.255.0");
  co_olsrd_add_hna(OLSR_HNA6, "0::0", "1::1");
  test_co_olsrd_print_conf("testing.conf");

  /* 
   * Remove an interface and an HNA
   */
  co_olsrd_remove_iface("eth2", OLSR_IFACE_ETHER, "255.255.2.0");
  co_olsrd_remove_hna(OLSR_HNA4, "192.168.1.1", "255.255.255.0");
  test_co_olsrd_print_conf("testing2.conf");

  /* 
   * Remove another interface and another HNA
   */
  co_olsrd_remove_iface("eth0", OLSR_IFACE_MESH, "255.255.255.0");
  co_olsrd_remove_hna(OLSR_HNA6, "0::0", "1::1");
  test_co_olsrd_print_conf("testing3.conf");
  return 0;
}
#endif
