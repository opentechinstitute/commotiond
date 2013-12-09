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
#include "list.h"

#include "servald.h"
#include "client.h"

/* Extension type */
#define _alarm 42
#define _timer 43
#define _socket 44

#define IS_ALARM(J) (IS_EXT(J) && ((co_alarm_t *)J)->_exttype == _alarm)
#define IS_TIMER(J) (IS_EXT(J) && ((co_timer_obj_t *)J)->_exttype == _timer)
#define IS_SOCK(J) (IS_EXT(J) && ((co_sock_obj_t *)J)->_exttype == _socket)

typedef struct {
  co_obj_t _header;
  uint8_t _exttype;
  uint8_t _len;
  struct sched_ent *alarm;
} co_alarm_t;

static int co_alarm_init(co_obj_t *output) {
  output->_type = _ext8;
  output->_next = NULL;
  output->_prev = NULL;
  ((co_alarm_t *)output)->_len = 0;
  ((co_alarm_t *)output)->_exttype = _alarm;
  return 1;
}

static co_obj_t *co_alarm_create(struct sched_ent *alarm) {
  co_alarm_t *output = h_calloc(1,sizeof(co_alarm_t));
  CHECK_MEM(output);
  CHECK(co_alarm_init((co_obj_t*)output),
      "Failed to initialize alarm.");
  output->_len = (sizeof(co_obj_t *) * 2);
  output->alarm = alarm;
  return (co_obj_t*)output;
error:
  return NULL;
}

typedef struct {
  co_obj_t _header;
  uint8_t _exttype;
  uint8_t _len;
  co_socket_t *sock;
} co_sock_obj_t;

static int co_sock_init(co_obj_t *output) {
  output->_type = _ext8;
  output->_next = NULL;
  output->_prev = NULL;
  ((co_sock_obj_t *)output)->_len = 0;
  ((co_sock_obj_t *)output)->_exttype = _socket;
  return 1;
}

static co_obj_t *co_sock_obj_create(co_socket_t *sock) {
  co_sock_obj_t *output = h_calloc(1,sizeof(co_sock_obj_t));
  CHECK_MEM(output);
  CHECK(co_sock_init((co_obj_t*)output),
	"Failed to initialize socket.");
  output->_len = (sizeof(co_obj_t *) * 2);
  output->sock = sock;
  return (co_obj_t*)output;
  error:
  return NULL;
}

typedef struct {
  co_obj_t _header;
  uint8_t _exttype;
  uint8_t _len;
  co_timer_t *timer;
} co_timer_obj_t;

static int co_timer_init(co_obj_t *output) {
  output->_type = _ext8;
  output->_next = NULL;
  output->_prev = NULL;
  ((co_timer_obj_t *)output)->_len = 0;
  ((co_timer_obj_t *)output)->_exttype = _timer;
  return 1;
}

static co_obj_t *co_timer_obj_create(co_timer_t *timer) {
  co_timer_obj_t *output = h_calloc(1,sizeof(co_timer_obj_t));
  CHECK_MEM(output);
  CHECK(co_timer_init((co_obj_t*)output),
	"Failed to initialize timer.");
  output->_len = (sizeof(co_obj_t *) * 2);
  output->timer = timer;
  return (co_obj_t*)output;
  error:
  return NULL;
}

extern co_timer_t co_timer_proto;

co_socket_t co_socket_proto = {};
static co_obj_t *sock_alarms = NULL;
static co_obj_t *timer_alarms = NULL;
static co_obj_t *socks = NULL;
static co_obj_t *timers = NULL;

/** Compares co_socket fd to serval alarm fd */
static co_obj_t *_alarm_fd_match_i(co_obj_t *alarms, co_obj_t *alarm, void *fd) {
  CHECK(IS_ALARM(alarm),"Invalid alarm");
  const struct sched_ent *this_alarm = ((co_alarm_t*)alarm)->alarm;
  const int *this_fd = fd;
  if (this_alarm->poll.fd == *this_fd) return alarm;
error:
  return NULL;
}

static co_obj_t *_socket_fd_match_i(co_obj_t *socks, co_obj_t *sock, void *fd) {
  CHECK(IS_SOCK(sock),"Invalid socket");
  const co_socket_t *this_sock = ((co_sock_obj_t*)sock)->sock;
  const int *this_fd = fd;
  char fd_str[6] = {0};
  sprintf(fd_str,"%d",*this_fd);
  if ((strcmp(this_sock->uri, fd_str)) == 0) return sock;
error:
  return NULL;
}

static co_obj_t *_alarm_ptr_match_i(co_obj_t *alarms, co_obj_t *alarm, void *ptr) {
  CHECK(IS_ALARM(alarm),"Invalid alarm");
  const struct sched_ent *this_alarm = ((co_alarm_t*)alarm)->alarm;
  const void *this_ptr = ptr;
  DEBUG("ALARM_PTR_MATCH: %p %p",this_alarm,this_ptr);
  if (this_alarm == this_ptr) return alarm;
error:
  return NULL;
}

static co_obj_t *_timer_ptr_match_i(co_obj_t *timers, co_obj_t *timer, void *ptr) {
  CHECK(IS_TIMER(timer),"Invalid timer");
  const co_timer_t *this_timer = ((co_timer_obj_t*)timer)->timer;
  const void *this_ptr = ptr;
  if (this_timer->ptr == this_ptr) return timer;
error:
  return NULL;
}

/** Callback function for when Serval socket has data to read */
int serval_socket_cb(void *self, void *context) {
  DEBUG("SERVAL_SOCKET_CB");
  co_socket_t *sock = self;
//   lnode_t *node = NULL;
  co_obj_t *node = NULL;
  struct sched_ent *alarm = NULL;
  
  // find alarm associated w/ sock, call alarm->function(alarm)
  if ((node = co_list_parse(sock_alarms, _alarm_fd_match_i, &sock->fd))) {
//   if ((node = list_find(sock_alarms, &sock->fd, alarm_fd_match))) {
//     alarm = lnode_get(node);
    alarm = ((co_alarm_t*)node)->alarm;
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
//   lnode_t *node = NULL;
  co_obj_t *node = NULL;
  struct sched_ent *alarm = NULL;
  
  // find alarm associated w/ timer, call alarm->function(alarm)
//   CHECK((node = list_find(timer_alarms, timer->ptr, alarm_ptr_match)),"Failed to find alarm for callback");
  CHECK((node = co_list_parse(timer_alarms, _alarm_ptr_match_i, timer->ptr)), "Failed to find alarm for callback");
//   alarm = lnode_get(node);
  alarm = ((co_alarm_t*)node)->alarm;
    
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
//   lnode_t *node = NULL;
  co_obj_t *node = NULL;
  
  CHECK(alarm->function,"No callback function associated with timer");
  CHECK(!(node = co_list_parse(timer_alarms, _alarm_ptr_match_i, alarm)),"Trying to schedule duplicate alarm %p",alarm);
//   CHECK(!(node = list_find(timer_alarms, alarm, alarm_ptr_match)),"Trying to schedule duplicate alarm %p",alarm);
//   CHECK(!(node = list_find(timers,alarm,timer_ptr_match)),"Timer for alarm already exists");
  CHECK(!(node = co_list_parse(timers,_timer_ptr_match_i,alarm)),"Timer for alarm already exists");
  
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
//   list_append(timer_alarms, lnode_create((void *)alarm));
  co_list_append(timer_alarms,co_alarm_create(alarm));

//   list_append(timers, lnode_create((void *)timer));
  co_list_append(timers,co_timer_obj_create(timer));
  
  return 0;
  
error:
  if (timer) free(timer);
  return -1;
}

/** Overridden Serval function to unschedule timed events */
int _unschedule(struct __sourceloc __whence, struct sched_ent *alarm) {
  DEBUG("OVERRIDDEN UNSCHEDULE FUNCTION!");
//   lnode_t *node = NULL;
  co_obj_t *node = NULL;
  co_timer_t *timer = NULL;
  
//   CHECK((node = list_find(timer_alarms, alarm, alarm_ptr_match)),"Attempting to unschedule timer that is not scheduled");
  CHECK((node = co_list_parse(timer_alarms, _alarm_ptr_match_i, alarm)),"Attempting to unschedule timer that is not scheduled");
//   list_delete(timer_alarms, node);
  co_list_delete(timer_alarms,node);
//   lnode_destroy(node);
  co_obj_free(node);
  DEBUG("# TIMER ALARMS: %lu",co_list_length(timer_alarms));
  
  // Get the timer associated with the alarm
//   CHECK((node = list_find(timers, alarm, timer_ptr_match)),"Could not find timer to remove");
  CHECK((node = co_list_parse(timers, _timer_ptr_match_i, alarm)),"Could not find timer to remove");
//   timer = lnode_get(node);
  timer = ((co_timer_obj_t*)node)->timer;
  if (!co_loop_remove_timer(timer,NULL))  // this is only useful when timers are unscheduled before expiring
    DEBUG("Failed to remove timer from event loop");
  free(timer);
//   list_delete(timers, node);
  co_list_delete(timers,node);
//   lnode_destroy(node);
  co_obj_free(node);
  
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
  
//   if ((alarm->_poll_index == 1) || list_find(sock_alarms, &alarm->poll.fd, alarm_fd_match)) {
  if ((alarm->_poll_index == 1) || co_list_parse(sock_alarms, _alarm_fd_match_i, &alarm->poll.fd)) {
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
    
//     list_append(sock_alarms, lnode_create((void *)alarm));
//     list_append(socks, lnode_create((void *)sock));
    co_list_append(sock_alarms, co_alarm_create(alarm));
    co_list_append(socks, co_sock_obj_create(sock));
  }
  
error:
  return 0;
}

int _unwatch(struct __sourceloc __whence, struct sched_ent *alarm) {
  DEBUG("OVERRIDDEN UNWATCH FUNCTION!");
//   lnode_t *node = NULL;
  co_obj_t *node = NULL;
  co_socket_t *sock = NULL;
  
//   CHECK(alarm->_poll_index == 1 && (node = list_find(sock_alarms, &alarm->poll.fd, alarm_fd_match)),"Attempting to unwatch socket that is not registered");
  CHECK(alarm->_poll_index == 1 && (node = co_list_parse(sock_alarms, _alarm_fd_match_i, &alarm->poll.fd)),"Attempting to unwatch socket that is not registered");
//   list_delete(sock_alarms, node);
  co_list_delete(sock_alarms,node);
  
  // Get the socket associated with the alarm (alarm's fd is equivalent to the socket's uri)
//   CHECK((node = list_find(socks, &alarm->poll.fd, socket_fd_match)),"Could not find socket to remove");
  CHECK((node = co_list_parse(socks, _socket_fd_match_i, &alarm->poll.fd)),"Could not find socket to remove");
//   sock = lnode_get(node);
  sock = ((co_sock_obj_t*)node)->sock;
  
//   list_delete(socks, node);
  co_list_delete(socks,node);
//   lnode_destroy(node);
  co_obj_free(node);
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
  
#define SCHEDULE(X, Y, D) { \
static struct profile_total _stats_##X={.name="" #X "",}; \
static struct sched_ent _sched_##X={\
.stats = &_stats_##X, \
.function=X,\
}; \
_sched_##X.alarm=(gettime_ms()+Y);\
_sched_##X.deadline=(gettime_ms()+Y+D);\
schedule(&_sched_##X); }
  
  /* Periodically check for new interfaces */
  SCHEDULE(overlay_interface_discover, 1, 100);
    
  /* Periodically advertise bundles */
  SCHEDULE(overlay_rhizome_advertise, 1000, 10000);
  
#undef SCHEDULE

}

co_obj_t *_name(co_obj_t *self, co_obj_t *params) {
  const char name[] = "servald";
  return co_str8_create(name,strlen(name),0);
}

void _init(void) {
  DEBUG("INIT");
  
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
//   sock_alarms = list_create(LOOP_MAXSOCK);
//   timer_alarms = list_create(LOOP_MAXSOCK);
//   socks = list_create(LOOP_MAXSOCK);
//   timers = list_create(LOOP_MAXTIMER);
  sock_alarms = co_list16_create();
  timer_alarms = co_list16_create();
  socks = co_list16_create();
  timers = co_list16_create();
  
  setup_sockets();
  
error:
  return;
}

// static void destroy_alarms(list_t *list, lnode_t *lnode, void *context) {
//   struct sched_ent *alarm = lnode_get(lnode);
//   struct __sourceloc nil;
//   if (alarm->alarm)
//     _unschedule(nil,alarm);
//   if (alarm->_poll_index)
//     _unwatch(nil,alarm);
//   return;
// }

static co_obj_t *destroy_alarms(co_obj_t *alarms, co_obj_t *alarm, void *context) {
  struct sched_ent *this_alarm = ((co_alarm_t*)alarm)->alarm;
  struct __sourceloc nil;
  if (this_alarm->alarm)
    _unschedule(nil,this_alarm);
  if (this_alarm->_poll_index)
    _unwatch(nil,this_alarm);
  return NULL;
}

void teardown(void) {
  DEBUG("TEARDOWN");
  
  servalShutdown = 1;
  
  dna_helper_shutdown();
  
//   list_process(sock_alarms,NULL,destroy_alarms);
//   list_destroy_nodes(sock_alarms);
//   list_destroy(sock_alarms);
//   
//   list_process(timer_alarms,NULL,destroy_alarms);
//   list_destroy_nodes(timer_alarms);
//   list_destroy(timer_alarms);
//   
//   list_destroy_nodes(socks);
//   list_destroy(socks);
//   
//   list_destroy_nodes(timers);
//   list_destroy(timers);
  
  co_list_parse(sock_alarms,destroy_alarms,NULL);
//   co_list_delete_all(sock_alarms);
  co_obj_free(sock_alarms);  // halloc will free list items
  
  co_list_parse(timer_alarms,destroy_alarms,NULL);
//   co_list_delete_all(timer_alarms);
  co_obj_free(timer_alarms); // halloc will free list items
  
//   co_list_delete_all(socks);
  co_obj_free(socks); // halloc will free list items
  
//   co_list_delete_all(timers);
  co_obj_free(timers); // halloc will free list items
  
  keyring_free(keyring);
  
  return;
}