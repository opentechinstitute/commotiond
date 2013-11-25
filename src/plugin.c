#define HAVE_ARPA_INET_H
#define HAVE_BCOPY

#include <stdlib.h>
#include <stdio.h>
#include <poll.h>
#include "serval/serval.h"
#include "serval/conf.h"

#include "debug.h"
#include "plugin.h"
#include "socket.h"
#include "loop.h"
#include "util.h"
#include "extern/list.h"

co_socket_t co_socket_proto = {};
static list_t *alarms = NULL;
static list_t *socks = NULL;

/** Compares co_socket fd to serval alarm fd */
static int alarm_match(const void *alarm, const void *fd) {
  const struct sched_ent *this_alarm = alarm;
  const int *this_fd = fd;
  if (this_alarm->poll.fd == *this_fd) return 0;
  return -1;
}

static int socket_match(const void *sock, const void *fd) {
  const co_socket_t *this_sock = sock;
  const int *this_fd = fd;
  char fd_str[6] = {0};
  sprintf(fd_str,"%d",*this_fd);
  if ((strcmp(this_sock->uri, fd_str)) == 0) return 0;
  return -1;
}

/** Callback function for when Serval socket has data to read */
int serval_cb(void *self, void *context) {
  printf("SERVAL_CB\n");
  co_socket_t *sock = self;
  lnode_t *node = NULL;
  struct sched_ent *alarm = NULL;
  
  // find alarm associated w/ sock, call alarm->function(alarm)
  if ((node = list_find(alarms, &sock->fd, alarm_match))) {
    alarm = lnode_get(node);
    alarm->poll.revents = POLLIN; /** Need to set this since Serval is using poll(2), but would
                                      probably be better to get the actual flags from the
                                      commotion event loop */
    
    printf("CALLING ALARM FUNC\n");
    alarm->function(alarm); // Serval callback function associated with alarm/socket
    return 0;
  }
  
  return -1;
}

/** Overridden Serval function to register sockets with event loop */
int _watch(struct __sourceloc __whence, struct sched_ent *alarm) {
  printf("OVERRIDDEN WATCH FUNCTION!\n");
  co_socket_t *sock = NULL;
  
  /** need to set:
   * 	sock->fd
   * 	sock->rfd
   * 	sock->listen
   * 	sock->uri
   * 	sock->poll_cb
   * 	sock->fd_registered
   * 	sock->rfd_registered
   */
  
  if ((alarm->_poll_index == 1) || list_find(alarms, &alarm->poll.fd, alarm_match)) {
    WARN("Socket %d already registered: %d",alarm->poll.fd,alarm->_poll_index);
  } else {
    sock = NEW(co_socket, co_socket);
    
    sock->fd = alarm->poll.fd;
    sock->rfd = 0;
    sock->listen = true;
//     sock->uri = NULL;
    // NOTE: Aren't able to get the Serval socket uris, so instead use string representation of fd
    sock->uri = (char*)malloc(6);
    sprintf(sock->uri,"%d",sock->fd);
    sock->fd_registered = false;
    sock->rfd_registered = false;
    sock->local = NULL;
    sock->remote = NULL;
  
    sock->poll_cb = serval_cb;
  
    // register sock
    CHECK(co_loop_add_socket(sock, NULL) == 1,"Failed to add socket %d",sock->fd);
  
    sock->fd_registered = true;
    // NOTE: it would be better to get the actual poll index from the event loop instead of this:
    alarm->_poll_index = 1;
    alarm->poll.revents = 0;
    
    list_append(alarms, lnode_create((void *)alarm));
    list_append(socks, lnode_create((void *)sock));
  }
  
error:
  return 0;
}

int _unwatch(struct __sourceloc __whence, struct sched_ent *alarm) {
  printf("OVERRIDDEN UNWATCH FUNCTION!\n");
  lnode_t *node = NULL;
  co_socket_t *sock = NULL;
  int ret = -1;
  
  CHECK(alarm->_poll_index == 1 && (node = list_find(alarms, &alarm->poll.fd, alarm_match)),"Attempting to unwatch socket that is not registered");
  list_delete(alarms, node);
  
  // Get the socket associated with the alarm (alarm's fd is equivalent to the socket's uri)
  CHECK((node = list_find(socks, &alarm->poll.fd, socket_match)),"Could not find socket to remove");
  sock = lnode_get(node);
  
  list_delete(socks, node);
  lnode_destroy(node);
  CHECK(co_loop_remove_socket(sock,NULL),"Failed to remove socket");
  CHECK(co_socket_destroy(sock),"Failed to destroy socket");
  
  alarm->_poll_index = -1;
  
  ret = 0;
  
error:
  return ret;
}

static void setup_sockets() {
  /* Setup up MDP & monitor interface unix domain sockets */
  printf("MDP SOCKETS\n");
  overlay_mdp_setup_sockets();
  
  printf("MONITOR SOCKETS\n");
  monitor_setup_sockets();
  
  printf("OLSR SOCKETS\n");
  olsr_init_socket();
  
  printf("RHIZOME\n");
  /* Get rhizome server started BEFORE populating fd list so that
   *  the server's listen so*cket is in the list for poll() */
  if (is_rhizome_enabled())
    rhizome_opendb();
  
  /* Rhizome http server needs to know which callback to attach
   *  t o client sockets, so pro*vide it here, along with the name to
   *  appear in time accounting statistics. */
  rhizome_http_server_start(rhizome_server_parse_http_request,
			    "rhizome_server_parse_http_request",
			    RHIZOME_HTTP_PORT,RHIZOME_HTTP_PORT_MAX);    
  
  printf("DNA HELPER\n");
  // start the dna helper if configured
  dna_helper_start();
  
  printf("DIRECTORY SERVICE\n");
  // preload directory service information
  directory_service_init();
  
  // TODO enable the following, will need to override Serval's _schedule/_unschedule functions:
  
  //   /* Periodically check for new interfaces */
  //   SCHEDULE(overlay_interface_discover, 1, 100);
  //   
  //   /* Periodically advertise bundles */
  //   SCHEDULE(overlay_rhizome_advertise, 1000, 10000);
}

void ready() {
  
  srandomdev();
  
  CHECK(cf_init() == 0, "Failed to initialize config");
  CHECK(cf_load_strict() == 1, "Failed to load config");
  CHECK(create_serval_instance_dir() != -1,"Unable to get/create Serval instance dir");
  CHECK(config.interfaces.ac != 0,"No network interfaces configured (empty 'interfaces' config option)");
  
  serverMode = 1;
  
  CHECK((keyring = keyring_open_instance()),"Could not open serval keyring file.");
  keyring_enter_pin(keyring, "");
  /* put initial identity in if we don't have any visible */	
  keyring_seed(keyring);
  
  overlay_queue_init();
  
  // Initialize our list of Serval alarms/sockets
  alarms = list_create(LOOP_MAXSOCK);
  socks = list_create(LOOP_MAXSOCK);
  
  setup_sockets();
  
error:
  return;
}

static void destroy_alarms(list_t *list, lnode_t *lnode, void *context) {
  struct sched_ent *alarm = lnode_get(lnode);
  struct __sourceloc nil;
  _unwatch(nil,alarm);
  return;
}

void teardown() {
  DEBUG("TEARDOWN");
  
  servalShutdown = 1;
  
  dna_helper_shutdown();
  
  list_process(alarms,NULL,destroy_alarms);
  list_destroy_nodes(alarms);
  list_destroy(alarms);
  
  keyring_free(keyring);
  
  return;
}