/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

/* ===========================================================================
 *
 * Modifications to this software (c) 2021 Disney
 *
 * ==========================================================================*/

#include "curl_setup.h"

#include <curl/curl.h>

#ifdef HAVE_NETINET_IN_H
#  include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IN6_H
#  include <netinet/in6.h>
#endif
#ifdef HAVE_NETDB_H
#  include <netdb.h>
#endif
#ifdef HAVE_ARPA_INET_H
#  include <arpa/inet.h>
#endif
#ifdef HAVE_SYS_UN_H
#  include <sys/un.h>
#endif

#ifdef __VMS
#  include <in.h>
#  include <inet.h>
#endif

#if defined(NETWARE) && defined(__NOVELL_LIBC__)
#  undef  in_addr_t
#  define in_addr_t unsigned long
#endif

#include <stddef.h>

#include "curl_addrinfo.h"
#include "inet_pton.h"
#include "warnless.h"
/* The last 3 #include files should be in this order */
#include "curl_printf.h"
#include "curl_memory.h"
#include "memdebug.h"

#if defined(USE_SB_SOCKET)
#include "source/adk/steamboat/sb_socket.h"
#endif

/*
 * Curl_freeaddrinfo()
 *
 * This is used to free a linked list of Curl_addrinfo structs along
 * with all its associated allocated storage. This function should be
 * called once for each successful call to Curl_getaddrinfo_ex() or to
 * any function call which actually allocates a Curl_addrinfo struct.
 */

#if defined(__INTEL_COMPILER) && (__INTEL_COMPILER == 910) && \
    defined(__OPTIMIZE__) && defined(__unix__) &&  defined(__i386__)
  /* workaround icc 9.1 optimizer issue */
# define vqualifier volatile
#else
# define vqualifier
#endif

void
Curl_freeaddrinfo(struct Curl_addrinfo *cahead)
{
  struct Curl_addrinfo *vqualifier canext;
  struct Curl_addrinfo *ca;

  for(ca = cahead; ca; ca = canext) {
    canext = ca->ai_next;
    free(ca);
  }
}


#ifdef HAVE_GETADDRINFO

#ifdef USE_SB_SOCKET
static sb_getaddrinfo_flags_e sys_to_sb_sock_flags(const int sys_flags) {
    return sys_flags == AI_PASSIVE ? sb_getaddrinfo_flag_ai_passive : sb_getaddrinfo_flag_none;
}

static sb_socket_protocol_type_e sys_to_sb_socket_protocol(const int sys_protocol) {
    sb_socket_protocol_type_e protocol = 0;

    switch (sys_protocol) {
        case IPPROTO_TCP:
            protocol = sb_socket_protocol_tcp;
            break;
        case IPPROTO_UDP:
            protocol = sb_socket_protocol_udp;
            break;
        case 0:
            protocol = sb_socket_protocol_any;
            break;
        default:
            protocol = sb_socket_protocol_unsupported;
    }
    return protocol;
}

static sb_socket_type_e sys_to_sb_sock_type(const int sys_sock) {
    sb_socket_type_e sock_type = 0;

    switch (sys_sock) {
        case SOCK_STREAM:
            sock_type = sb_socket_type_stream;
            break;
        case SOCK_DGRAM:
            sock_type = sb_socket_type_datagram;
            break;
        case 0:
            sock_type = sb_socket_type_any;
            break;
        default:
            sock_type = sb_socket_type_unsupported;
    }
    return sock_type;
}

static sb_socket_family_e sys_to_sb_sock_family(const int sys_domain) {
    sb_socket_family_e domain = 0;
    switch (sys_domain) {
        case AF_INET:
            domain = sb_socket_family_IPv4;
            break;
        case AF_INET6:
            domain = sb_socket_family_IPv6;
            break;
        case AF_UNSPEC:
            domain = sb_socket_family_dont_care;
            break;
        default:
            domain = sb_socket_family_unsupported;
    }
    return domain;
}

static void sb_addrinfo_to_sockaddr_storage(const sb_addrinfo_t *const sb_addrinfo, struct sockaddr_storage * const out_sockaddr_storage) {
  ASSERT(sb_addrinfo);
  ASSERT(out_sockaddr_storage);
  memset(out_sockaddr_storage, 0, sizeof(struct sockaddr_storage));
  switch(sb_addrinfo->ai_family) {
      case sb_socket_family_IPv4: {
          struct sockaddr_in * ipv4_sockaddr = (struct sockaddr_in *)out_sockaddr_storage;
          ipv4_sockaddr->sin_family = AF_INET;
          ipv4_sockaddr->sin_port = sb_addrinfo->ai_addr.sin_port;
          memcpy(&ipv4_sockaddr->sin_addr, &sb_addrinfo->ai_addr.sin_addr, sizeof(ipv4_sockaddr->sin_addr));
        break;
      }
      case sb_socket_family_IPv6: {
        struct sockaddr_in6 * ipv6_sockaddr = (struct sockaddr_in6 *)out_sockaddr_storage;
        ipv6_sockaddr->sin6_family = AF_INET6;
        ipv6_sockaddr->sin6_port = sb_addrinfo->ai_addr.sin_port;
        ipv6_sockaddr->sin6_flowinfo = sb_addrinfo->ai_addr.ipv6_flowinfo;
        ipv6_sockaddr->sin6_scope_id = sb_addrinfo->ai_addr.ipv6_scope_id;
        memcpy(&ipv6_sockaddr->sin6_addr, &sb_addrinfo->ai_addr.sin_addr, sizeof(ipv6_sockaddr->sin6_addr));
        break;
      }
      default:
            TRAP("Invalid socket family %i", sb_addrinfo->ai_family);
    }
}

#endif
/*
 * Curl_getaddrinfo_ex()
 *
 * This is a wrapper function around system's getaddrinfo(), with
 * the only difference that instead of returning a linked list of
 * addrinfo structs this one returns a linked list of Curl_addrinfo
 * ones. The memory allocated by this function *MUST* be free'd with
 * Curl_freeaddrinfo().  For each successful call to this function
 * there must be an associated call later to Curl_freeaddrinfo().
 *
 * There should be no single call to system's getaddrinfo() in the
 * whole library, any such call should be 'routed' through this one.
 */

int
Curl_getaddrinfo_ex(const char *nodename,
                    const char *servname,
                    const struct addrinfo *hints,
                    struct Curl_addrinfo **result)
{
  struct addrinfo *ai;
  struct addrinfo *aihead;
  struct Curl_addrinfo *cafirst = NULL;
  struct Curl_addrinfo *calast = NULL;
  struct Curl_addrinfo *ca;
  size_t ss_size;
  int error;

  *result = NULL; /* assume failure */

#if defined(USE_SB_SOCKET)
  struct addrinfo sys_addrinfo;
  struct sockaddr_storage sys_sockaddr;
  sb_addrinfo_t in_hints;
  sb_addrinfo_t out_addrinfo[10];
  uint32_t inout_addrinfo_size = 10;

  aihead = NULL;

  in_hints.ai_protocol = sys_to_sb_socket_protocol(hints->ai_protocol);
  in_hints.ai_socktype = sys_to_sb_sock_type(hints->ai_socktype);
  in_hints.ai_family = sys_to_sb_sock_family(hints->ai_family);
  in_hints.ai_flags = sys_to_sb_sock_flags(hints->ai_flags);
  error = sb_getaddrinfo(nodename, servname, &in_hints, out_addrinfo, &inout_addrinfo_size).system_code;
#else
  error = getaddrinfo(nodename, servname, hints, &aihead);
#endif

  if(error)
    return error;

  /* traverse the addrinfo list */

#if defined(USE_SB_SOCKET)
  for (uint32_t addrindex = 0; addrindex < inout_addrinfo_size; addrindex++) {
    ai = (struct addrinfo *)hints;
    sb_addrinfo_to_sockaddr_storage(&out_addrinfo[addrindex], &sys_sockaddr);
    ai->ai_addr = (struct sockaddr *)&sys_sockaddr;
    if(ai->ai_family == AF_INET)
        ai->ai_addrlen = sizeof(struct sockaddr_in);
  #ifdef ENABLE_IPV6
      else if(ai->ai_family == AF_INET6)
        ai->ai_addrlen = sizeof(struct sockaddr_in6);
  #endif
      else
        continue;
#else
  for(ai = aihead; ai != NULL; ai = ai->ai_next) {
#endif
    size_t namelen = ai->ai_canonname ? strlen(ai->ai_canonname) + 1 : 0;
    /* ignore elements with unsupported address family, */
    /* settle family-specific sockaddr structure size.  */
    if(ai->ai_family == AF_INET)
      ss_size = sizeof(struct sockaddr_in);
#ifdef ENABLE_IPV6
    else if(ai->ai_family == AF_INET6)
      ss_size = sizeof(struct sockaddr_in6);
#endif
    else
      continue;

    /* ignore elements without required address info */
    if((ai->ai_addr == NULL) || !(ai->ai_addrlen > 0))
      continue;

    /* ignore elements with bogus address size */
    if((size_t)ai->ai_addrlen < ss_size)
      continue;

    ca = malloc(sizeof(struct Curl_addrinfo) + ss_size + namelen);
    if(!ca) {
      error = EAI_MEMORY;
      break;
    }

    /* copy each structure member individually, member ordering, */
    /* size, or padding might be different for each platform.    */

    ca->ai_flags     = ai->ai_flags;
    ca->ai_family    = ai->ai_family;
    ca->ai_socktype  = ai->ai_socktype;
    ca->ai_protocol  = ai->ai_protocol;
    ca->ai_addrlen   = (curl_socklen_t)ss_size;
    ca->ai_addr      = NULL;
    ca->ai_canonname = NULL;
    ca->ai_next      = NULL;

    ca->ai_addr = (void *)((char *)ca + sizeof(struct Curl_addrinfo));
    memcpy(ca->ai_addr, ai->ai_addr, ss_size);

    if(namelen) {
      ca->ai_canonname = (void *)((char *)ca->ai_addr + ss_size);
      memcpy(ca->ai_canonname, ai->ai_canonname, namelen);
    }

    /* if the return list is empty, this becomes the first element */
    if(!cafirst)
      cafirst = ca;

    /* add this element last in the return list */
    if(calast)
      calast->ai_next = ca;
    calast = ca;

  }

  /* destroy the addrinfo list */
  if(aihead)
    freeaddrinfo(aihead);

  /* if we failed, also destroy the Curl_addrinfo list */
  if(error) {
    Curl_freeaddrinfo(cafirst);
    cafirst = NULL;
  }
  else if(!cafirst) {
#ifdef EAI_NONAME
    /* rfc3493 conformant */
    error = EAI_NONAME;
#else
    /* rfc3493 obsoleted */
    error = EAI_NODATA;
#endif
#ifdef USE_WINSOCK
    SET_SOCKERRNO(error);
#endif
  }

  *result = cafirst;

  /* This is not a CURLcode */
  return error;
}
#endif /* HAVE_GETADDRINFO */


/*
 * Curl_he2ai()
 *
 * This function returns a pointer to the first element of a newly allocated
 * Curl_addrinfo struct linked list filled with the data of a given hostent.
 * Curl_addrinfo is meant to work like the addrinfo struct does for a IPv6
 * stack, but usable also for IPv4, all hosts and environments.
 *
 * The memory allocated by this function *MUST* be free'd later on calling
 * Curl_freeaddrinfo().  For each successful call to this function there
 * must be an associated call later to Curl_freeaddrinfo().
 *
 *   Curl_addrinfo defined in "lib/curl_addrinfo.h"
 *
 *     struct Curl_addrinfo {
 *       int                   ai_flags;
 *       int                   ai_family;
 *       int                   ai_socktype;
 *       int                   ai_protocol;
 *       curl_socklen_t        ai_addrlen;   * Follow rfc3493 struct addrinfo *
 *       char                 *ai_canonname;
 *       struct sockaddr      *ai_addr;
 *       struct Curl_addrinfo *ai_next;
 *     };
 *
 *   hostent defined in <netdb.h>
 *
 *     struct hostent {
 *       char    *h_name;
 *       char    **h_aliases;
 *       int     h_addrtype;
 *       int     h_length;
 *       char    **h_addr_list;
 *     };
 *
 *   for backward compatibility:
 *
 *     #define h_addr  h_addr_list[0]
 */

struct Curl_addrinfo *
Curl_he2ai(const struct hostent *he, int port)
{
  struct Curl_addrinfo *ai;
  struct Curl_addrinfo *prevai = NULL;
  struct Curl_addrinfo *firstai = NULL;
  struct sockaddr_in *addr;
#ifdef ENABLE_IPV6
  struct sockaddr_in6 *addr6;
#endif
  CURLcode result = CURLE_OK;
  int i;
  char *curr;

  if(!he)
    /* no input == no output! */
    return NULL;

  DEBUGASSERT((he->h_name != NULL) && (he->h_addr_list != NULL));

  for(i = 0; (curr = he->h_addr_list[i]) != NULL; i++) {
    size_t ss_size;
    size_t namelen = strlen(he->h_name) + 1; /* include zero termination */
#ifdef ENABLE_IPV6
    if(he->h_addrtype == AF_INET6)
      ss_size = sizeof(struct sockaddr_in6);
    else
#endif
      ss_size = sizeof(struct sockaddr_in);

    /* allocate memory to told the struct, the address and the name */
    ai = calloc(1, sizeof(struct Curl_addrinfo) + ss_size + namelen);
    if(!ai) {
      result = CURLE_OUT_OF_MEMORY;
      break;
    }
    /* put the address after the struct */
    ai->ai_addr = (void *)((char *)ai + sizeof(struct Curl_addrinfo));
    /* then put the name after the address */
    ai->ai_canonname = (char *)ai->ai_addr + ss_size;
    memcpy(ai->ai_canonname, he->h_name, namelen);

    if(!firstai)
      /* store the pointer we want to return from this function */
      firstai = ai;

    if(prevai)
      /* make the previous entry point to this */
      prevai->ai_next = ai;

    ai->ai_family = he->h_addrtype;

    /* we return all names as STREAM, so when using this address for TFTP
       the type must be ignored and conn->socktype be used instead! */
    ai->ai_socktype = SOCK_STREAM;

    ai->ai_addrlen = (curl_socklen_t)ss_size;

    /* leave the rest of the struct filled with zero */

    switch(ai->ai_family) {
    case AF_INET:
      addr = (void *)ai->ai_addr; /* storage area for this info */

      memcpy(&addr->sin_addr, curr, sizeof(struct in_addr));
      addr->sin_family = (CURL_SA_FAMILY_T)(he->h_addrtype);
      addr->sin_port = htons((unsigned short)port);
      break;

#ifdef ENABLE_IPV6
    case AF_INET6:
      addr6 = (void *)ai->ai_addr; /* storage area for this info */

      memcpy(&addr6->sin6_addr, curr, sizeof(struct in6_addr));
      addr6->sin6_family = (CURL_SA_FAMILY_T)(he->h_addrtype);
      addr6->sin6_port = htons((unsigned short)port);
      break;
#endif
    }

    prevai = ai;
  }

  if(result) {
    Curl_freeaddrinfo(firstai);
    firstai = NULL;
  }

  return firstai;
}


struct namebuff {
  struct hostent hostentry;
  union {
    struct in_addr  ina4;
#ifdef ENABLE_IPV6
    struct in6_addr ina6;
#endif
  } addrentry;
  char *h_addr_list[2];
};


/*
 * Curl_ip2addr()
 *
 * This function takes an internet address, in binary form, as input parameter
 * along with its address family and the string version of the address, and it
 * returns a Curl_addrinfo chain filled in correctly with information for the
 * given address/host
 */

struct Curl_addrinfo *
Curl_ip2addr(int af, const void *inaddr, const char *hostname, int port)
{
  struct Curl_addrinfo *ai;

#if defined(__VMS) && \
    defined(__INITIAL_POINTER_SIZE) && (__INITIAL_POINTER_SIZE == 64)
#pragma pointer_size save
#pragma pointer_size short
#pragma message disable PTRMISMATCH
#endif

  struct hostent  *h;
  struct namebuff *buf;
  char  *addrentry;
  char  *hoststr;
  size_t addrsize;

  DEBUGASSERT(inaddr && hostname);

  buf = malloc(sizeof(struct namebuff));
  if(!buf)
    return NULL;

  hoststr = strdup(hostname);
  if(!hoststr) {
    free(buf);
    return NULL;
  }

  switch(af) {
  case AF_INET:
    addrsize = sizeof(struct in_addr);
    addrentry = (void *)&buf->addrentry.ina4;
    memcpy(addrentry, inaddr, sizeof(struct in_addr));
    break;
#ifdef ENABLE_IPV6
  case AF_INET6:
    addrsize = sizeof(struct in6_addr);
    addrentry = (void *)&buf->addrentry.ina6;
    memcpy(addrentry, inaddr, sizeof(struct in6_addr));
    break;
#endif
  default:
    free(hoststr);
    free(buf);
    return NULL;
  }

  h = &buf->hostentry;
  h->h_name = hoststr;
  h->h_aliases = NULL;
  h->h_addrtype = (short)af;
  h->h_length = (short)addrsize;
  h->h_addr_list = &buf->h_addr_list[0];
  h->h_addr_list[0] = addrentry;
  h->h_addr_list[1] = NULL; /* terminate list of entries */

#if defined(__VMS) && \
    defined(__INITIAL_POINTER_SIZE) && (__INITIAL_POINTER_SIZE == 64)
#pragma pointer_size restore
#pragma message enable PTRMISMATCH
#endif

  ai = Curl_he2ai(h, port);

  free(hoststr);
  free(buf);

  return ai;
}

/*
 * Given an IPv4 or IPv6 dotted string address, this converts it to a proper
 * allocated Curl_addrinfo struct and returns it.
 */
struct Curl_addrinfo *Curl_str2addr(char *address, int port)
{
  struct in_addr in;
  if(Curl_inet_pton(AF_INET, address, &in) > 0)
    /* This is a dotted IP address 123.123.123.123-style */
    return Curl_ip2addr(AF_INET, &in, address, port);
#ifdef ENABLE_IPV6
  {
    struct in6_addr in6;
    if(Curl_inet_pton(AF_INET6, address, &in6) > 0)
      /* This is a dotted IPv6 address ::1-style */
      return Curl_ip2addr(AF_INET6, &in6, address, port);
  }
#endif
  return NULL; /* bad input format */
}

#ifdef USE_UNIX_SOCKETS
/**
 * Given a path to a Unix domain socket, return a newly allocated Curl_addrinfo
 * struct initialized with this path.
 * Set '*longpath' to TRUE if the error is a too long path.
 */
struct Curl_addrinfo *Curl_unix2addr(const char *path, bool *longpath,
                                     bool abstract)
{
  struct Curl_addrinfo *ai;
  struct sockaddr_un *sa_un;
  size_t path_len;

  *longpath = FALSE;

  ai = calloc(1, sizeof(struct Curl_addrinfo) + sizeof(struct sockaddr_un));
  if(!ai)
    return NULL;
  ai->ai_addr = (void *)((char *)ai + sizeof(struct Curl_addrinfo));

  sa_un = (void *) ai->ai_addr;
  sa_un->sun_family = AF_UNIX;

  /* sun_path must be able to store the NUL-terminated path */
  path_len = strlen(path) + 1;
  if(path_len > sizeof(sa_un->sun_path)) {
    free(ai);
    *longpath = TRUE;
    return NULL;
  }

  ai->ai_family = AF_UNIX;
  ai->ai_socktype = SOCK_STREAM; /* assume reliable transport for HTTP */
  ai->ai_addrlen = (curl_socklen_t)
    ((offsetof(struct sockaddr_un, sun_path) + path_len) & 0x7FFFFFFF);

  /* Abstract Unix domain socket have NULL prefix instead of suffix */
  if(abstract)
    memcpy(sa_un->sun_path + 1, path, path_len - 1);
  else
    memcpy(sa_un->sun_path, path, path_len); /* copy NUL byte */

  return ai;
}
#endif

#if defined(CURLDEBUG) && defined(HAVE_GETADDRINFO) &&  \
  defined(HAVE_FREEADDRINFO)
/*
 * curl_dbg_freeaddrinfo()
 *
 * This is strictly for memory tracing and are using the same style as the
 * family otherwise present in memdebug.c. I put these ones here since they
 * require a bunch of structs I didn't want to include in memdebug.c
 */

void
curl_dbg_freeaddrinfo(struct addrinfo *freethis,
                      int line, const char *source)
{
  curl_dbg_log("ADDR %s:%d freeaddrinfo(%p)\n",
               source, line, (void *)freethis);
#ifdef USE_LWIPSOCK
  lwip_freeaddrinfo(freethis);
#else
  (freeaddrinfo)(freethis);
#endif
}
#endif /* defined(CURLDEBUG) && defined(HAVE_FREEADDRINFO) */


#if defined(CURLDEBUG) && defined(HAVE_GETADDRINFO)
/*
 * curl_dbg_getaddrinfo()
 *
 * This is strictly for memory tracing and are using the same style as the
 * family otherwise present in memdebug.c. I put these ones here since they
 * require a bunch of structs I didn't want to include in memdebug.c
 */

int
curl_dbg_getaddrinfo(const char *hostname,
                    const char *service,
                    const struct addrinfo *hints,
                    struct addrinfo **result,
                    int line, const char *source)
{
#ifdef USE_LWIPSOCK
  int res = lwip_getaddrinfo(hostname, service, hints, result);
#else
  int res = (getaddrinfo)(hostname, service, hints, result);
#endif
  if(0 == res)
    /* success */
    curl_dbg_log("ADDR %s:%d getaddrinfo() = %p\n",
                 source, line, (void *)*result);
  else
    curl_dbg_log("ADDR %s:%d getaddrinfo() failed\n",
                 source, line);
  return res;
}
#endif /* defined(CURLDEBUG) && defined(HAVE_GETADDRINFO) */

#if defined(HAVE_GETADDRINFO) && defined(USE_RESOLVE_ON_IPS)
/*
 * Work-arounds the sin6_port is always zero bug on iOS 9.3.2 and Mac OS X
 * 10.11.5.
 */
void Curl_addrinfo_set_port(struct Curl_addrinfo *addrinfo, int port)
{
  struct Curl_addrinfo *ca;
  struct sockaddr_in *addr;
#ifdef ENABLE_IPV6
  struct sockaddr_in6 *addr6;
#endif
  for(ca = addrinfo; ca != NULL; ca = ca->ai_next) {
    switch(ca->ai_family) {
    case AF_INET:
      addr = (void *)ca->ai_addr; /* storage area for this info */
      addr->sin_port = htons((unsigned short)port);
      break;

#ifdef ENABLE_IPV6
    case AF_INET6:
      addr6 = (void *)ca->ai_addr; /* storage area for this info */
      addr6->sin6_port = htons((unsigned short)port);
      break;
#endif
    }
  }
}
#endif
