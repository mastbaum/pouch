#ifndef __POUCH_H__
#define __POUCH_H__

// Standard libraries
#include <sys/stat.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>

// Libcurl
#include <curl/curl.h>

// Defines
#define USE_SYS_FILE 0
#define GET "GET"
#define PUT "PUT"
#define POST "POST"
#define HEAD "HEAD"
#define COPY "COPY"
#define DELETE "DELETE"

// Structs
typedef struct _PouchPkt PouchPkt;
typedef struct _PouchReq PouchReq;

/** _PouchPkt
 *
 *   Holds data to be sent to or received from a CouchDB server
 */
struct _PouchPkt {
    char *data;
    char *offset;
    size_t size;
};

/** _PouchReq
 *
 *  A structure to be used to send a request to a CouchDB server and save the
 *  response, as well as any error codes.
 */
struct _PouchReq {
    CURL *easy;			// CURL easy request
    CURLcode curlcode;	// CURL easy interface error code
    CURLM *multi;		// CURL multi object
    CURLMcode curlmcode; // CURLM multi interface error code
    char errorstr[CURL_ERROR_SIZE]; // holds an error description
    struct curl_slist *headers;	// Custom headers for uploading
    char *method;		// HTTP method
    char *url;			// Destination (e.g., "http://127.0.0.1:5984/test");
    char *usrpwd;		// Holds a user:password authentication string
    long httpresponse;	// holds the http response of a request
    PouchPkt req;		// holds data to be sent
    PouchPkt resp;		// holds response
};

// Miscellaneous helper functions

/** URL escapes a string. Use this to escape database names. */
char *url_escape(CURL *curl, char *str);

/** Appends the strings f, sep, and s, in that order, and copies the result to
 * out.
 *
 * If a separator is unnecessary, call the function with sep=NULL. *out  must
 * have been malloced() or initiated as NULL.
 */
char *combine(char **out, char *f, char *s, char *sep);

/** Stores the current revision of the document in pr->resp.data.
 *
 * If you want to do anything with that revision string, make sure to copy it
 * to another place in memory before reusing the request.
 */
char *doc_get_cur_rev(PouchReq *pr, char *server, char *db, char *id);

// PouchReq functions

/** Initialize a new PouchReq object */
PouchReq *pr_init(void);

/** Add a custom header to a request */
PouchReq *pr_add_header(PouchReq *pr, char *h);

PouchReq *pr_add_usrpwd(PouchReq *pr, char *usrpwd, size_t length);

/** Add a parameter to a request's URL string, regardless of whether other
 *  parameters already exist
 */
PouchReq *pr_add_param(PouchReq *pr, char *key, char *value);

/** Remove all parameters from a request's URL string, if they exist */
PouchReq *pr_clear_params(PouchReq *pr);

/** Set the HTTP method of the request */
PouchReq *pr_set_method(PouchReq *pr, char *method);

/** Set the target URL of a request */
PouchReq *pr_set_url(PouchReq *pr, char *url);

/** Set the data that a request sends. If the request does not need to send
 *  data, do not call this function. */
PouchReq *pr_set_data(PouchReq *pr, char *str);

PouchReq *pr_set_prdata(PouchReq *pr, char *str, size_t len);
PouchReq *pr_set_bdata(PouchReq *pr, void *dat, size_t length);

/** Remove all data from a request's buffer, if it exists */
PouchReq *pr_clear_data(PouchReq *pr);
PouchReq *pr_do(PouchReq *pr);
PouchReq *pr_domulti(PouchReq *pr, CURLM *multi);

/** Free any memory allocated during the creation or processing of a request.
 *  While it is okay to reuse requests, always call this when finished to avoid
 *  leaks.
 */
void pr_free(PouchReq *pr);

// Database Wrapper Functions

/** Get a list of all databases on a server */
PouchReq *get_all_dbs(PouchReq *p_req, char *server);

/** Delete database 'db' on a server */
PouchReq *db_delete(PouchReq *p_req, char *server, char *db);

/** Create a database 'db' on a server */
PouchReq *db_create(PouchReq *p_req, char *server, char *db);

/** Get information about the database */
PouchReq *db_get(PouchReq *p_req, char *server, char *db);

/** Get a list of changes to a document in the database */
PouchReq *db_get_changes(PouchReq *pr, char *server, char *db);

/** Get the maximum number of revisions allowed for documents */
PouchReq *db_get_revs_limit(PouchReq *pr, char *server, char *db);

/** Set the maximum number of revisions allowed for documents */
PouchReq *db_set_revs_limit(PouchReq *pr, char *server, char *db, char *revs);

/** Initiate a compaction of the database */
PouchReq *db_compact(PouchReq *pr, char *server, char *db);

// Document Wrapper Functions

/** Retrieve a document */
PouchReq *doc_get(PouchReq *pr, char *server, char *db, char *id);

/** Retrieve a specific revision of a document */
PouchReq *doc_get_rev(PouchReq *pr, char *server, char *db, char *id, char *rev);

/** Find out what revisions are available for a document. Returns the current
 *  revision of the document, but with an additional field, _revisions, the
 *  value being a list of the available revision IDs.
 */
PouchReq *doc_get_revs(PouchReq *pr, char *server, char *db, char *id);

/** Make a HEAD request, returning basic information about the document,
 *  including its current revision
 */
PouchReq *doc_get_info(PouchReq *pr, char *server, char *db, char *id);

/** Create a new document with an automatically generated revision ID. The
 *  JSON body must include a _id property which contains a unique id. If the
 *  document already exists, and the JSON data body includes a _rev property,
 *  then the document is updated.
 */
PouchReq *doc_create_id(PouchReq *pr, char *server, char *db, char *id, char *data);

/** Create a new document with a server-generated id */
PouchReq *doc_create(PouchReq *pr, char *server, char *db, char *data);

PouchReq *doc_prcreate(PouchReq *pr, char *server, char *db, char *data);

/** Return all of the documents in a database */
PouchReq *get_all_docs(PouchReq *pr, char *server, char *db);

/** Return all documents that have been updated or deleted, in the order they
 *  were modified
 */
PouchReq *get_all_docs_by_seq(PouchReq *pr, char *server, char *db);

/** Get an attachment on a document */
PouchReq *doc_get_attachment(PouchReq *pr, char *server, char *db, char *id, char *name);

/** Copy a document from one id to another */
PouchReq *doc_copy(PouchReq *pr, char *server, char *db, char *id, char *newid, char *revision);

/** Delete a document and all attachments. The revision number is required. */
PouchReq *doc_delete(PouchReq *pr, char *server, char *db, char *id, char *rev);

/** Upload a file as an attachment to a document */
PouchReq *doc_add_attachment(PouchReq *pr, char *server, char *db, char *doc, char *filename);

// Generic curl callback functions

/** Callback used to save CURL requests. Loads the response into
 *  PouchReq* data 
 */
size_t recv_data_callback(char *ptr, size_t size, size_t nmemb, void *data);

/** Callback used to send a CURL request. The JSON string stored in
 *  PouchReq* data is read out and sent.
 */
size_t send_data_callback(void *ptr, size_t size, size_t nmemb, void *data);

#endif

