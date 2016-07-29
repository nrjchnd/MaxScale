# REST API design document

This document contains the design for the MaxScale REST API.
Resources

The main objects in MaxScale are service, server, monitor and filter. These objects are represented as individual sections in the configuration file and can exist alone.
Additionally, the current maxadmin commands that produce diagnostic output can be thought of as read-only resources.

Even though the listeners are defined as separate entities in the configuration file, they are actually a part of the service. This can be seen from the fact that a listener always requires a service. Internally, they are just DCBs of a different type.

## Resources that support GET

The response to a GET request will be a JSON representation of the requested resource. The structure of the returned JSON object is dependant on the resource being requested and the current configuration of MaxScale (e.g. servers can have multiple weighting parameters).

### /service/:id

Returns information about a specific service.

#### Common fields

All services will return these fields in the response. Services can return extra fields depending on the routing module in use.

|Field              |Type        |Value                                         |
|-------------------|------------|----------------------------------------------|
|name               |string      |Service name                                  |
|router             |string      |Router module                                 |
|state              |string      |Service state                                 |
|total_connections  |number      |Total number of connection to this service    |
|current_connections|number      |Current number of connection to this service  |
|started            |string      |When the service was started                  |
|servers            |string array|The servers used by this service              |

### /service

Returns information about all services. Returns an array of JSON objects similar to the `service/:id` resource.

### /server/:id

Returns information about a specific server.

#### Common fields

All servers will return these fields in the response. In addition to these fields, it is also possible that servers return extra fields e.g. the weighting parameter.

|Field    |Type  |Value             |
|---------|------|------------------|
|name     |string|Server name       |
|address  |string|Server address    |
|port     |number|Server port       |
|protocol |string|Server protocol   |

### /server

Returns information about all servers. Comparable to the output of maxadmin show servers.

### /monitor/:id

Returns information about a specific monitor.

#### Common fields

All monitors will return these fields in the response. In addition to these fields, it is also possible that the monitor returns extra fields.

|Field              |Type        |Value                                         |
|-------------------|------------|----------------------------------------------|
|name               |string      |Monitor name                                  |
|module             |string      |Monitor module                                |
|state              |string      |Monitor state                                 |
|servers            |string array|The servers used by this monitor              |
|monitor_interval   |number      |The monitoring interval in milliseconds       |
|connect_timeout    |number      |Connect timeout for backend connections       |
|read_timeout       |number      |Read timeout for backend connections          |
|write_timeout      |number      |Write timeout for backend connections         |

### /monitor

Returns information about all monitors. Comparable to the output of maxadmin show monitors.

### /filter

Returns information about all filters. Comparable to the output of maxadmin show filters.

### /filter/:id

Returns information about a specific filer. Comparable to the output of maxadmin show filter <name>.

### /dcb

Returns information about all DCBs. Comparable to the output of maxadmin show dcbs.

### /session

Returns information about all sessions. Comparable to the output of maxadmin show sessions.

### /task

Returns information about all currently running housekeeping tasks. Comparable to the output of maxadmin show tasks.

### /thread

Returns information about threads. Comparable to the combined output of maxadmin show [threads|epoll|eventq|eventstats].

### /module

Returns information about currently loaded modules. Comparable to the output of maxadmin show modules.

# Resources that support PUT

All PUT requests that fail due to errors in the provided data receive a response with the HTTP response code 400. If the failure is due to an internal error, the HTTP response code 500 is returned.

### /server/:id

Modifies the server whose name matches the :id component of the URL. The body of the PUT statement must represent the new server configuration in JSON and should have all the required fields.

#### Required fields

|Field|Type|Description|
|-----|----|-----------|
|name|string|Server name|
|address|string|Server network address|
|port|number|Server port
|protocol|string|Backend server protocol e.g. MySQLBackend|
