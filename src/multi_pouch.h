#ifndef __MULTI_POUCH_H__
#define __MULTI_POUCH_H__

// Standard libraries
#include <sys/stat.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>

// Libevent and Libcurl
#include <event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/dns.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <curl/curl.h>

// Pouch helpers
#include "pouch.h"

// Defines
#define USE_SYS_FILE 0
#define GET "GET"
#define PUT "PUT"
#define POST "POST"
#define HEAD "HEAD"
#define COPY "COPY"
#define DELETE "DELETE"

// Structs
typedef struct _SockInfo SockInfo;
typedef struct _PouchMInfo PouchMInfo;

/** pr_proc_cb
 *
 *  Callback function for processing finished PouchReqs
 *
 *  If a pr_proc_cb is set by the user, that function becomes responsible for
 *  pr_free()'ing the received PouchReq.
 */
typedef void (*pr_proc_cb)(PouchReq *, PouchMInfo *);

/** SockInfo
 *
 *  Holds information on a socket used for performing different easy_requests.
 *
 *  Used in the multi interface only. 
 *
 *  Stolen from http://curl.haxx.se/libcurl/c/hiperfifo.html
 */
struct _SockInfo {
    curl_socket_t sockfd; // socket to be monitored
    struct event ev;      // event on the socket
    int ev_is_set;        // whether or not ev is set and being monitored
    int action;           // what action libcurl wants done
};

/** _PouchMInfo
 *
 *  Holds values necessary for using libevent with libcurl; used for the multi
 *  interface. This is the struct that gets passed around to all of the
 *  libevent and libcurl callback functions.
 *
 *  Used in the multi interface only.
 */
struct _PouchMInfo {
    CURLM *multi;
    struct event timer_event;	// event necessary for libevent to work with libcurl
    struct event_base *base;	// libevent event_base for creating events
    struct evdns_base *dnsbase ;// libevent dns_base for creating connections
    int still_running;			// whether or not there are any running libcurl handles
    pr_proc_cb cb;		// USER DEFINED pointer to a callback function for processing finished PouchReqs
    int has_cb;			// ... tests for existence of callback function
    void *custom;				// USER DEFINED pointer to some data. 
};

// libevent/libcurl multi interface helpers and callbacks
void debug_mcode(const char *desc, CURLMcode code);
void check_multi_info(PouchMInfo *pmi /*, function pointer process_func*/);

/** Update the event timer after curl_multi library calls */
int multi_timer_cb(CURLM *multi, long timeout_ms, void *data);

/** Called by libevent when there is any type of action on a watched socket */
void event_cb(int fd, short kind, void *userp);

/** Called by libevent when the global timeout that is used to take action on
 *  easy_handles expires
 */
void timer_cb(int fd, short kind, void *userp);

/** Set up a SockInfo structure and start libevent monitoring on a socket */
void setsock(SockInfo *fdp, curl_socket_t s, int action, PouchMInfo *pmi);

/** The CURLMOPT_SOCKETFUNCTION. This is what tells libevent to start or stop
 *   watching for events on different sockets.
 *
 *   e      = easy handle that the callback happened on,
 *   s      = actual socket that is involved,
 *   action = what to check for / do (?) (nothing, IN, OUT, INOUT, REMOVE)
 *   cbp    = private data set by curl_multi_setop(CURLMOPT_SOCKETDATA)
 *   sockp  = private data set by curl_multi_assign(multi, socket, sockp)
 */
int sock_cb(CURL *e, curl_socket_t s, int action, void *cbp, void *sockp);

/** Initialize a PouchMInfo structure for use with libevent.
 *
 *  This creates and initializes the CURLM handle pointer, as well as the
 *  libevent timer_event which is used to deal with the multi interface, but
 *  takes in a pointer to a libevent event_base for use as a sort of "global"
 *  base throughout all of the callbacks, so that the user can define their own
 *  base.
 */
PouchMInfo *pr_mk_pmi(struct event_base *base, struct evdns_base *dns_base, pr_proc_cb callback, void *custom);

void pmi_multi_cleanup(PouchMInfo *pmi);

/** Cleans up and deletes a PouchMInfo struct.
 * 
 *  It gets rid of the timer event, frees the event base (don't do this
 *  manually after calling pr_del_pmi!) and cleans up the CURLM handle.
 *  Afterwards, it frees the object. Don't try to free it again.
 */
void pr_del_pmi(PouchMInfo *pmi);

#endif

