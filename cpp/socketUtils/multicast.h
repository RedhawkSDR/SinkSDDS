#ifndef MULTICAST_H_
#define MULTICAST_H_

#include <arpa/inet.h>
#include <stdexcept>
#include "SourceNicUtils.h"
#include "unicast.h"

#ifdef __cplusplus
extern "C" {
#endif

connection_t multicast_client (const char* iface, const char* group, int port) throw (BadParameterError);
ssize_t multicast_receive (connection_t client, void* buffer, size_t bytes);
connection_t multicast_server (const char* iface, const char* group, int port);
ssize_t multicast_transmit (connection_t server, const void* buffer, size_t bytes);
int multicast_poll_in (connection_t client, int timeout);
void multicast_close(connection_t socket);

#ifdef __cplusplus
}
#endif

#endif /* MULTICAST_H_ */
