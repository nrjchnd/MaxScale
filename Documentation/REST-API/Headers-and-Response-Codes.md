# Headers and Response Codes

## Request Headers

REST makes use of the HTTP protocols in its aim to provide a natural way to understand the workings of an API. The following request headers are understood by this API.

TODO: Add the rest

### Date

this header is required and should be in the standard form, which is defined by RFC822, an example of which is Mon, 18 Nov 2013 08:14:29 -0600. Please note that an RFC 2822 date must be in English. It will be checked by the API for being close to the current date and time

## Response Headers

TODO: Figure these out

## Response Codes

Every HTTP response starts with a line with a return code which indicates the outcome of the request. The API uses some of the standard HTTP values:

* 200 OK
* 204 No Content
* 400 Bad Request – this includes validation failures
* 401 Unauthorized
* 404 Not Found
* 405 Method Not Allowed
* 409 Conflict – the request is inconsistent with known constraints
* 500 Internal Server Error
