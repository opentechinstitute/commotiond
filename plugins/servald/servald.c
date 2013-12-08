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

#define SCHEDULE(X, Y, D) { \
static struct profile_total _stats_##X={.name="" #X "",}; \
static struct sched_ent _sched_##X={\
.stats = &_stats_##X, \
.function=X,\
}; \
_sched_##X.alarm=(gettime_ms()+Y);\
_sched_##X.deadline=(gettime_ms()+Y+D);\
schedule(&_sched_##X); }

extern co_timer_t co_timer_proto;

co_socket_t co_socket_proto = {};
static list_t *sock_alarms = NULL;
static list_t *timer_alarms = NULL;
static list_t *socks = NULL;
static list_t *timers = NULL;

/** Compares co_socket fd to serval alarm fd */
static int alarm_fd_match(const void *alarm, const void *fd) {
  const struct sched_ent *this_alarm = alarm;
  const int *this_fd = fd;
  if (this_alarm->poll.fd == *this_fd) return 0;
  return -1;
}

static int socket_fd_match(const void *sock, const void *fd) {
  const co_socket_t *this_sock = sock;
  const int *this_fd = fd;
  char fd_str[6] = {0};
  sprintf(fd_str,"%d",*this_fd);
  if ((strcmp(this_sock->uri, fd_str)) == 0) return 0;
  return -1;
}

static int alarm_ptr_match(const void *alarm, const void *ptr) {
  const struct sched_ent *this_alarm = alarm;
  const void *this_ptr = ptr;
  DEBUG("ALARM_PTR_MATCH: %p %p",this_alarm,this_ptr);
  if (this_alarm == this_ptr) return 0;
  return -1;
}

static int timer_ptr_match(const void *timer, const void *ptr) {
  const co_timer_t *this_timer = timer;
  if (this_timer->ptr == ptr) return 0;
  return -1;
}

/** Callback function for when Serval socket has data to read */
int serval_socket_cb(void *self, void *context) {
  DEBUG("SERVAL_SOCKET_CB");
  co_socket_t *sock = self;
  lnode_t *node = NULL;
  struct sched_ent *alarm = NULL;
  
  // find alarm associated w/ sock, call alarm->function(alarm)
  if ((node = list_find(sock_alarms, &sock->fd, alarm_fd_match))) {
    alarm = lnode_get(node);
    alarm->poll.revents = POLLIN; /** Need to set this since Serval is using poll(2), but would
                                      probably be better to get the actual flags from the
                                      commotion event loop */
    
    DEBUG("CALLING ALARM FUNC");
    alarm->function(alarm); // Serval callback function associated with alarm/socket
    return 0;
  }
  
  return -1;
}

int serval_timer_cb(void *self, void *context) {
  DEBUG("SERVAL_TIMER_CB");
  co_timer_t *timer = self;
  lnode_t *node = NULL;
  struct sched_ent *alarm = NULL;
  
  // find alarm associated w/ timer, call alarm->function(alarm)
  CHECK((node = list_find(timer_alarms, timer->ptr, alarm_ptr_match)),"Failed to find alarm for callback");
  alarm = lnode_get(node);
    
  struct __sourceloc nil;
  DEBUG("PENDING: %d",timer->pending);
  _unschedule(nil,alarm);
    
  DEBUG("CALLING TIMER FUNC");
  alarm->function(alarm); // Serval callback function associated with alarm/socket
  return 0;

error:
  return -1;
}

/** Overridden Serval function to schedule timed events */
int _schedule(struct __sourceloc __whence, struct sched_ent *alarm) {
  DEBUG("OVERRIDDEN SCHEDULE FUNCTION!");
  co_timer_t *timer = NULL;
  lnode_t *node = NULL;
  
  CHECK(alarm->function,"No callback function associated with timer");
  CHECK(!(node = list_find(timer_alarms, alarm, alarm_ptr_match)),"Trying to schedule duplicate alarm %p",alarm);
  CHECK(!(node = list_find(timers,alarm,timer_ptr_match)),"Timer for alarm already exists");
  
  if (alarm->deadline < alarm->alarm)
    alarm->deadline = alarm->alarm;
  
  time_ms_t now = gettime_ms();
  
  CHECK(now - alarm->deadline <= 1000,"Alarm tried to schedule a deadline %lldms ago, from %s() %s:%d",
	 (now - alarm->deadline),
	 __whence.function,__whence.file,__whence.line);
  
  // if the alarm has already expired, execute callback function
  if (alarm->alarm <= now) {
    DEBUG("ALREADY EXPIRED, DEADLINE %lld %lld",alarm->alarm, gettime_ms());
    alarm->function(alarm);
    return 0;
  }
  
  timer = NEW(co_timer,co_timer);
  timer->ptr = alarm;
  DEBUG("NEW TIMER: ALARM %lld %p",alarm->alarm - now,alarm);
  time_ms_t deadline;
  if (alarm->alarm - now > 1)
    deadline = alarm->alarm;
  else
    deadline = alarm->deadline;
  timer->timer_cb = serval_timer_cb;
  timer->deadline.tv_sec = deadline / 1000;
  timer->deadline.tv_usec = (deadline % 1000) * 1000;
  if (timer->deadline.tv_usec > 1000000) {
    timer->deadline.tv_sec++;
    timer->deadline.tv_usec %= 1000000;
  }
  CHECK(co_loop_add_timer(timer,(void*)NULL),"Failed to add timer %ld.%06ld %p",timer->deadline.tv_sec,timer->deadline.tv_usec,timer->ptr);
  list_append(timer_alarms, lnode_create((void *)alarm));

  list_append(timers, lnode_create((void *)timer));
  
  return 0;
  
error:
  if (timer) free(timer);
  return -1;
}

/** Overridden Serval function to unschedule timed events */
int _unschedule(struct __sourceloc __whence, struct sched_ent *alarm) {
  DEBUG("OVERRIDDEN UNSCHEDULE FUNCTION!");
  lnode_t *node = NULL;
  co_timer_t *timer = NULL;
  
  CHECK((node = list_find(timer_alarms, alarm, alarm_ptr_match)),"Attempting to unschedule timer that is not scheduled");
  list_delete(timer_alarms, node);
  lnode_destroy(node);
  DEBUG("# TIMER ALARMS: %lu",list_count(timer_alarms));
  
  // Get the timer associated with the alarm
  CHECK((node = list_find(timers, alarm, timer_ptr_match)),"Could not find timer to remove");
  timer = lnode_get(node);
  if (!co_loop_remove_timer(timer,NULL))  // this is only useful when timers are unscheduled before expiring
    DEBUG("Failed to remove timer from event loop");
  free(timer);
  list_delete(timers, node);
  lnode_destroy(node);
  
  return 0;
  
error:
  return -1;
}

/** Overridden Serval function to register sockets with event loop */
int _watch(struct __sourceloc __whence, struct sched_ent *alarm) {
  DEBUG("OVERRIDDEN WATCH FUNCTION!");
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
  
  if ((alarm->_poll_index == 1) || list_find(sock_alarms, &alarm->poll.fd, alarm_fd_match)) {
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
  
    sock->poll_cb = serval_socket_cb;
  
    // register sock
    CHECK(co_loop_add_socket(sock, NULL) == 1,"Failed to add socket %d",sock->fd);
  
    sock->fd_registered = true;
    // NOTE: it would be better to get the actual poll index from the event loop instead of this:
    alarm->_poll_index = 1;
    alarm->poll.revents = 0;
    
    list_append(sock_alarms, lnode_create((void *)alarm));
    list_append(socks, lnode_create((void *)sock));
  }
  
error:
  return 0;
}

int _unwatch(struct __sourceloc __whence, struct sched_ent *alarm) {
  DEBUG("OVERRIDDEN UNWATCH FUNCTION!");
  lnode_t *node = NULL;
  co_socket_t *sock = NULL;
  
  CHECK(alarm->_poll_index == 1 && (node = list_find(sock_alarms, &alarm->poll.fd, alarm_fd_match)),"Attempting to unwatch socket that is not registered");
  list_delete(sock_alarms, node);
  
  // Get the socket associated with the alarm (alarm's fd is equivalent to the socket's uri)
  CHECK((node = list_find(socks, &alarm->poll.fd, socket_fd_match)),"Could not find socket to remove");
  sock = lnode_get(node);
  
  list_delete(socks, node);
  lnode_destroy(node);
  CHECK(co_loop_remove_socket(sock,NULL),"Failed to remove socket");
  CHECK(co_socket_destroy(sock),"Failed to destroy socket");
  
  alarm->_poll_index = -1;
  
  return 0;
  
error:
  return -1;
}

static void setup_sockets() {
  /* Setup up MDP & monitor interface unix domain sockets */
  DEBUG("MDP SOCKETS");
  overlay_mdp_setup_sockets();
  
  DEBUG("MONITOR SOCKETS");
  monitor_setup_sockets();
  
  DEBUG("OLSR SOCKETS");
  olsr_init_socket();
  
  DEBUG("RHIZOME");
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
  
  DEBUG("DNA HELPER");
  // start the dna helper if configured
  dna_helper_start();
  
  DEBUG("DIRECTORY SERVICE");
  // preload directory service information
  directory_service_init();
  
  /* Periodically check for new interfaces */
  SCHEDULE(overlay_interface_discover, 1, 100);
    
  /* Periodically advertise bundles */
  SCHEDULE(overlay_rhizome_advertise, 1000, 10000);
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
  sock_alarms = list_create(LOOP_MAXSOCK);
  timer_alarms = list_create(LOOP_MAXSOCK);
  socks = list_create(LOOP_MAXSOCK);
  timers = list_create(LOOP_MAXTIMER);
  
  setup_sockets();
  
error:
  return;
}

static void destroy_alarms(list_t *list, lnode_t *lnode, void *context) {
  struct sched_ent *alarm = lnode_get(lnode);
  struct __sourceloc nil;
  if (alarm->alarm)
    _unschedule(nil,alarm);
  if (alarm->_poll_index)
    _unwatch(nil,alarm);
  return;
}

void teardown() {
  DEBUG("TEARDOWN");
  
  servalShutdown = 1;
  
  dna_helper_shutdown();
  
  list_process(sock_alarms,NULL,destroy_alarms);
  list_destroy_nodes(sock_alarms);
  list_destroy(sock_alarms);
  
  list_process(timer_alarms,NULL,destroy_alarms);
  list_destroy_nodes(timer_alarms);
  list_destroy(timer_alarms);
  
  list_destroy_nodes(socks);
  list_destroy(socks);
  
  list_destroy_nodes(timers);
  list_destroy(timers);
  
  keyring_free(keyring);
  
  return;
}