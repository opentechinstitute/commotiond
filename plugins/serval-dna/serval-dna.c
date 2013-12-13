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
#include "tree.h"
#include "profile.h"

#include "serval-dna.h"
#include "client.h"
#include "crypto.h"

#define DEFAULT_SID "0000000000000000000000000000000000000000000000000000000000000000"
#define DEFAULT_MDP_PATH "/etc/commotion/keys.d/mdp.keyring/serval.keyring"
#define DEFAULT_SERVAL_PATH "/var/serval-node"

// Types & constructors

/* Extension type */
#define _alarm 42

#define IS_ALARM(J) (IS_EXT(J) && ((co_alarm_t *)J)->_exttype == _alarm)

typedef struct {
  co_obj_t _header;
  uint8_t _exttype;
  uint8_t _len;
  struct sched_ent *alarm;
} co_alarm_t;

static co_obj_t *co_alarm_create(struct sched_ent *alarm) {
  co_alarm_t *output = h_calloc(1,sizeof(co_alarm_t));
  CHECK_MEM(output);
  output->_header._type = _ext8;
  output->_exttype = _alarm;
  output->_len = (sizeof(co_alarm_t));
  output->alarm = alarm;
  return (co_obj_t*)output;
error:
  return NULL;
}

// Globals

extern keyring_file *mdp_keyring;
extern unsigned char *mdp_key;
extern int mdp_key_len;

char *serval_path = NULL;
co_socket_t co_socket_proto = {};

static co_obj_t *sock_alarms = NULL;
static co_obj_t *timer_alarms = NULL;
// static co_obj_t *socks = NULL;
// static co_obj_t *timers = NULL;

// Private functions

/** Compares co_socket fd to serval alarm fd */
static co_obj_t *_alarm_fd_match_i(co_obj_t *alarms, co_obj_t *alarm, void *fd) {
  if(!IS_ALARM(alarm)) return NULL;
  const struct sched_ent *this_alarm = ((co_alarm_t*)alarm)->alarm;
  const int *this_fd = fd;
  if (this_alarm->poll.fd == *this_fd) return alarm;
  return NULL;
}

/*
static co_obj_t *_socket_fd_match_i(co_obj_t *socks, co_obj_t *sock, void *fd) {
  if(!IS_SOCK(sock)) return NULL;
  const co_socket_t *this_sock = (co_socket_t*)sock;
  const int *this_fd = fd;
  char fd_str[6] = {0};
  sprintf(fd_str,"%d",*this_fd);
  if ((strcmp(this_sock->uri, fd_str)) == 0) return sock;
  return NULL;
}
*/

static co_obj_t *_alarm_ptr_match_i(co_obj_t *alarms, co_obj_t *alarm, void *ptr) {
  if(!IS_ALARM(alarm)) return NULL;
  const struct sched_ent *this_alarm = ((co_alarm_t*)alarm)->alarm;
  const void *this_ptr = ptr;
  DEBUG("ALARM_PTR_MATCH: %p %p",this_alarm,this_ptr);
  if (this_alarm == this_ptr) return alarm;
  return NULL;
}

/*
static co_obj_t *_timer_ptr_match_i(co_obj_t *timers, co_obj_t *timer, void *ptr) {
  if(!IS_TIMER(timer)) return NULL;
  const co_timer_t *this_timer = (co_timer_t*)timer;
  const void *this_ptr = ptr;
  if (this_timer->ptr == this_ptr) return timer;
  return NULL;
}
*/

// Public functions

/** Callback function for when Serval socket has data to read */
int serval_socket_cb(co_obj_t *self, co_obj_t *context) {
  DEBUG("SERVAL_SOCKET_CB");
  co_socket_t *sock = (co_socket_t*)self;
  co_obj_t *node = NULL;
  struct sched_ent *alarm = NULL;
  
  // find alarm associated w/ sock, call alarm->function(alarm)
  if ((node = co_list_parse(sock_alarms, _alarm_fd_match_i, &sock->fd))) {
    alarm = ((co_alarm_t*)node)->alarm;
    alarm->poll.revents = POLLIN; /** Need to set this since Serval is using poll(2), but would
                                      probably be better to get the actual flags from the
                                      commotion event loop */
    
    DEBUG("CALLING ALARM FUNC");
    alarm->function(alarm); // Serval callback function associated with alarm/socket
    return 1;
  }
  
  return 0;
}

int serval_timer_cb(co_obj_t *self, co_obj_t **output, co_obj_t *context) {
  DEBUG("SERVAL_TIMER_CB");
  co_obj_t *timer = self;
  co_obj_t *node = NULL;
  struct sched_ent *alarm = NULL;
  co_obj_t *alarm_node = NULL;
  
  // find alarm associated w/ timer, call alarm->function(alarm)
//   CHECK((node = co_list_parse(timer_alarms, _alarm_ptr_match_i, ((co_timer_t*)timer)->ptr)), "Failed to find alarm for callback");
  if ((node = co_list_parse(timer_alarms, _alarm_ptr_match_i, ((co_timer_t*)timer)->ptr)) == NULL) {
    ERROR("Failed to find alarm for callback");
  }
  alarm = ((co_alarm_t*)node)->alarm;
  
  co_obj_free(timer);
    
//   struct __sourceloc nil;
//   _unschedule(nil,alarm);
  CHECK((alarm_node = co_list_parse(timer_alarms, _alarm_ptr_match_i, alarm)),"Attempting to remove timer that is not scheduled");
  co_list_delete(timer_alarms,alarm_node);
  co_obj_free(alarm_node);
  DEBUG("# TIMER ALARMS: %lu",co_list_length(timer_alarms));
    
  DEBUG("CALLING TIMER FUNC");
  alarm->function(alarm); // Serval callback function associated with alarm/socket

  return 0;
error:
  return 1;
}

static co_obj_t *asd(co_obj_t *list, co_obj_t *current, void *context) {
  int *a = context;
  
  if (!IS_ALARM(current)) return NULL;
  (*a)++;
  DEBUG("##### %p -> %p",current,((co_alarm_t*)current)->alarm);
  return NULL;
}

/** Overridden Serval function to schedule timed events */
int _schedule(struct __sourceloc __whence, struct sched_ent *alarm) {
  DEBUG("OVERRIDDEN SCHEDULE FUNCTION!");
  co_obj_t *timer = NULL, *node = NULL;
  
  CHECK(alarm->function,"No callback function associated with timer");
  CHECK(!(node = co_list_parse(timer_alarms, _alarm_ptr_match_i, alarm)),"Trying to schedule duplicate alarm %p",alarm);
//   CHECK(!(node = co_list_parse(timers,_timer_ptr_match_i,alarm)),"Timer for alarm already exists");
  
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
  
  DEBUG("NEW TIMER: ALARM %lld - %lld = %lld %p",alarm->alarm,now,alarm->alarm - now,alarm);
  DEBUG("$$$$$$$$$$$$$ callback: %p",alarm->function);
  CHECK(alarm->alarm - now < 86400000,"Timer deadline is more than 24 hrs from now, ignoring");
  time_ms_t deadline_time;
  struct timeval deadline;
  if (alarm->alarm - now > 1)
    deadline_time = alarm->alarm;
  else
    deadline_time = alarm->deadline;
  deadline.tv_sec = deadline_time / 1000;
  deadline.tv_usec = (deadline_time % 1000) * 1000;
  if (deadline.tv_usec > 1000000) {
    deadline.tv_sec++;
    deadline.tv_usec %= 1000000;
  }
  timer = co_timer_create(deadline, serval_timer_cb, alarm);
  CHECK(co_loop_add_timer(timer,NULL),"Failed to add timer %ld.%06ld %p",deadline.tv_sec,deadline.tv_usec,alarm);
  DEBUG("Successfully added timer %ld.%06ld %p",deadline.tv_sec,deadline.tv_usec,alarm);
  int a = 0;
  co_obj_t *the_alarm = NULL;
  the_alarm = co_alarm_create(alarm);
  CHECK(the_alarm,"FAIL");
  CHECK(co_list_append(timer_alarms,the_alarm),"Failed to add to timer_alarms");
  co_list_parse(timer_alarms,asd,&a);
  if (a != co_list_length(timer_alarms)) {
    DEBUG("a = %d, len(timer_alarms) = %ld",a,co_list_length(timer_alarms));
    a;
  }
  

//   CHECK(co_list_append(timers,timer),"Failed to add to timers");
  
  return 0;
  
error:
  if (timer) free(timer);
  return -1;
}

/** Overridden Serval function to unschedule timed events */
int _unschedule(struct __sourceloc __whence, struct sched_ent *alarm) {
  DEBUG("OVERRIDDEN UNSCHEDULE FUNCTION!");
  co_obj_t *alarm_node = NULL, *timer = NULL;
  
  CHECK((alarm_node = co_list_parse(timer_alarms, _alarm_ptr_match_i, alarm)),"Attempting to unschedule timer that is not scheduled");
  co_list_delete(timer_alarms,alarm_node);
  co_obj_free(alarm_node);
  DEBUG("# TIMER ALARMS: %lu",co_list_length(timer_alarms));
  
  // Get the timer associated with the alarm
  CHECK((timer = co_loop_get_timer(alarm,NULL)),"Could not find timer to remove");
//   if ((timer = co_loop_get_timer(alarm,NULL))) {
//   CHECK((timer = co_list_parse(timers, _timer_ptr_match_i, alarm)),"Could not find timer to remove");
//     if (!co_loop_remove_timer(timer,NULL))  // this is only useful when timers are unscheduled before expiring
//       WARN("Failed to remove timer from event loop");
  CHECK(co_loop_remove_timer(timer,NULL),"Failed to remove timer from event loop");
//   co_list_delete(timers,timer);
    co_obj_free(timer);
//   }
  
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
  
  if ((alarm->_poll_index == 1) || co_list_parse(sock_alarms, _alarm_ptr_match_i, alarm)) {
    WARN("Socket %d already registered: %d",alarm->poll.fd,alarm->_poll_index);
  } else {
    sock = (co_socket_t*)NEW(co_socket, co_socket);
    
    sock->fd = alarm->poll.fd;
    sock->rfd = 0;
    sock->listen = true;
    // NOTE: Aren't able to get the Serval socket uris, so instead use string representation of fd
    sock->uri = h_calloc(1,6);
    hattach(sock->uri,sock);
    sprintf(sock->uri,"%d",sock->fd);
    sock->fd_registered = false;
    sock->rfd_registered = false;
    sock->local = NULL;
    sock->remote = NULL;
  
    sock->poll_cb = serval_socket_cb;
  
    // register sock
    CHECK(co_loop_add_socket((co_obj_t*)sock, NULL) == 1,"Failed to add socket %d",sock->fd);
    DEBUG("Successfully added socket %d",sock->fd);
  
    sock->fd_registered = true;
    // NOTE: it would be better to get the actual poll index from the event loop instead of this:
    alarm->_poll_index = 1;
    alarm->poll.revents = 0;
    
    CHECK(co_list_append(sock_alarms, co_alarm_create(alarm)),"Failed to add to sock_alarms");
    DEBUG("Successfully added to sock_alarms %p",alarm);
//     DEBUG("socks size: %ld",co_list_length(socks));
//     CHECK(co_list_append(socks, (co_obj_t*)sock),"Failed to add to socks: %p",(co_obj_t*)sock);
//     DEBUG("Successfully added to socks: %p",(co_obj_t*)sock);
  }
  
  return 0;
error:
  return -1;
}

int _unwatch(struct __sourceloc __whence, struct sched_ent *alarm) {
  DEBUG("OVERRIDDEN UNWATCH FUNCTION!");
  co_obj_t *node = NULL;
  char fd_str[6] = {0};
  
  CHECK(alarm->_poll_index == 1 && (node = co_list_parse(sock_alarms, _alarm_ptr_match_i, alarm)),"Attempting to unwatch socket that is not registered");
  co_list_delete(sock_alarms,node);
  
  // Get the socket associated with the alarm (alarm's fd is equivalent to the socket's uri)
  sprintf(fd_str,"%d",alarm->poll.fd);
  CHECK((node = co_loop_get_socket(fd_str,NULL)),"Could not find socket to remove");
//   CHECK((node = co_list_parse(socks, _socket_fd_match_i, &alarm->poll.fd)),"Could not find socket to remove");
//   co_list_delete(socks,node);
  CHECK(co_loop_remove_socket(node,NULL),"Failed to remove socket");
  CHECK(co_socket_destroy(node),"Failed to destroy socket");
  
  alarm->_poll_index = -1;
  
  return 0;
  
error:
  return -1;
}

static void setup_sockets(void) {
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
  
  DEBUG("finished setup_sockets");
  
#undef SCHEDULE

}

int co_plugin_name(co_obj_t *self, co_obj_t **output, co_obj_t *params) {
  const char name[] = "serval-dna";
  CHECK((*output = co_str8_create(name,strlen(name)+1,0)),"Failed to create plugin name");
  return 1;
error:
  return 0;
}

int serval_schema(co_obj_t *self, co_obj_t **output, co_obj_t *params) {
  DEBUG("Loading serval schema.");
  co_tree_insert(self, "serval_path", strlen("serval_path"), co_str8_create(DEFAULT_SERVAL_PATH, sizeof(DEFAULT_SERVAL_PATH), 0));
  co_tree_insert(self, "mdp_sid", strlen("mdp_sid"), co_str8_create(DEFAULT_SID, sizeof(DEFAULT_SID), 0));
  co_tree_insert(self, "mdp_keyring", strlen("mdp_keyring"), co_str8_create(DEFAULT_MDP_PATH, sizeof(DEFAULT_MDP_PATH), 0));
  return 1;
}

int co_plugin_register(co_obj_t *self, co_obj_t **output, co_obj_t *params) {
  CHECK(co_schema_register(serval_schema),"Failed to register plugin schema");
  return 1;
error:
  return 0;
}

int co_plugin_init(co_obj_t *self, co_obj_t **output, co_obj_t *params) {
  DEBUG("INIT");
  int ret = 0, mdp_sid_len, mdp_path_len;
  co_obj_t *global_str = NULL, *global = NULL;
  char *mdp_sid = NULL, *mdp_path = NULL;
  unsigned char packedSid[SID_SIZE] = {0};
  
  global_str = co_str8_create("global.profile",sizeof("global.profile")-1,0);
  CHECK((global = co_profile_find(global_str)),"Failed to fetch global profile");
  
  mdp_sid_len = co_profile_get_str(global,&mdp_sid,"mdp_sid",7);
  CHECK(mdp_sid_len == 2*SID_SIZE && str_is_subscriber_id(mdp_sid) == 1,"Invalid mdp_sid config parameter");
  
  mdp_path_len = co_profile_get_str(global,&mdp_path,"mdp_keyring",11);
  CHECK(mdp_path_len < PATH_MAX,"mdp_keyring config parameter too long");
  
  DEBUG("mdp_sid: %s",mdp_sid);
  DEBUG("mdp_path: %s",mdp_path);
  DEBUG("mdp_path_len: %d",mdp_path_len);
  
  stowSid(packedSid,0,mdp_sid);
  CHECK(_serval_init(packedSid,
			 SID_SIZE,
			 mdp_path,
			 mdp_path_len,
			 &mdp_keyring,
			 &mdp_key,
			 &mdp_key_len), "Failed to initialize olsrd-mdp Serval keyring");
  
  CHECK(co_profile_get_str(global,&serval_path,"serval_path",11) < PATH_MAX - 16,"serval_path config parameter too long");
  CHECK(setenv("SERVALINSTANCE_PATH",serval_path,1) == 0,"Failed to set SERVALINSTANCE_PATH env variable");
  
  DEBUG("serval_path: %s",serval_path);
  
  CHECK(serval_register(),"Failed to register Serval commands");
  CHECK(serval_crypto_register(),"Failed to register Serval-crypto commands");
  CHECK(olsrd_mdp_register(),"Failed to register OLSRd-mdp commands");
  
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
  sock_alarms = co_list16_create();
  timer_alarms = co_list16_create();
//   socks = co_list16_create();
//   timers = co_list16_create();
  
  setup_sockets();
  
  ret = 1;
error:
  co_obj_free(global_str);
  return ret;
}

static co_obj_t *destroy_alarms(co_obj_t *alarms, co_obj_t *alarm, void *context) {
  struct sched_ent *this_alarm = ((co_alarm_t*)alarm)->alarm;
  struct __sourceloc nil;
  if (this_alarm->alarm)
    _unschedule(nil,this_alarm);
  if (this_alarm->_poll_index)
    _unwatch(nil,this_alarm);
  return NULL;
}

int co_plugin_shutdown(co_obj_t *self, co_obj_t **output, co_obj_t *params) {
  DEBUG("TEARDOWN");
  
  if (mdp_keyring) keyring_free(mdp_keyring);
  
  servalShutdown = 1;
  
  dna_helper_shutdown();
  
  co_list_parse(sock_alarms,destroy_alarms,NULL);
  co_obj_free(sock_alarms);  // halloc will free list items
  
  co_list_parse(timer_alarms,destroy_alarms,NULL);
  co_obj_free(timer_alarms); // halloc will free list items
  
//   co_obj_free(socks); // halloc will free list items
  
//   co_obj_free(timers); // halloc will free list items
  
  keyring_free(keyring);
  
  return 1;
}