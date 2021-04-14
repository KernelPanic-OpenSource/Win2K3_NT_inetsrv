;/**********************************************************************/
;/**                       Microsoft Windows NT                       **/
;/**                Copyright(c) Microsoft Corp., 1993                **/
;/**********************************************************************/
;
;/*
;    w3msg.h
;
;    This file is generated by the MC tool from the W3MSG.MC message
;    file.
;
;
;    FILE HISTORY:
;        KeithMo     19-Mar-1993 Created.
;
;*/
;
;
;#ifndef _W3MSG_H_
;#define _W3MSG_H_
;

SeverityNames=(Success=0x0
               Informational=0x1
               Warning=0x2
               Error=0x3
              )

Messageid=2201 Severity=Error SymbolicName=W3_EVENT_CANNOT_INITIALIZE_SECURITY
Language=English
HTTP Server could not initialize its security.  The data is the error.
.

Messageid=2203 Severity=Error SymbolicName=W3_EVENT_CANNOT_INITIALIZE_WINSOCK
Language=English
HTTP Server could not initialize the socket library.  The data is the error.
.

Messageid=2204 Severity=Error SymbolicName=W3_EVENT_OUT_OF_MEMORY
Language=English
HTTP Server was unable to initialize due to a shortage of available memory.  The data is the error.
.

Messageid=2206 Severity=Error SymbolicName=W3_EVENT_CANNOT_CREATE_CONNECTION_SOCKET
Language=English
HTTP Server could not create the main connection socket.  The data is the error.
.

Messageid=2208 Severity=Error SymbolicName=W3_EVENT_CANNOT_CREATE_CLIENT_CONN
Language=English
HTTP Server could not create a client connection object for user at host %1.  The connection to this user is terminated.  The data is the error.
.

Messageid=2214 Severity=Error SymbolicName=W3_EVENT_FILTER_DLL_LOAD_FAILED
Language=English
The HTTP Filter DLL %1 failed to load.  The data is the error.
.

Messageid=2216 Severity=Error SymbolicName=W3_EVENT_KILLING_SCRIPT
Language=English
The script started from the URL '%1' with parameters '%2' has not responded within the configured timeout period.  The HTTP server is terminating the script.
.

Messageid=2218 Severity=Error SymbolicName=W3_MSG_SSI_ERROR
Language=English
The HTTP server encountered an error processing the server side include file '%1'.<BR>The error was '%2'.<BR>
.

Messageid=2219 Severity=Error SymbolicName=W3_EVENT_EXTENSION_EXCEPTION
Language=English
The HTTP server encountered an unhandled exception while processing the ISAPI Application '%1'.
.

Messageid=2220 Severity=Error SymbolicName=W3_EVENT_EXTENSION_LOAD_FAILED
Language=English
The HTTP server was unable to load the ISAPI Application '%1'. The data is the error.
.

Messageid=2221 Severity=Error SymbolicName=W3_MSG_SSI_TOO_MANY_NESTED_INCLUDES
Language=English
A server side include file has included itself or the maximum depth of server side includes has been exceeded.
.

Messageid=2222 Severity=Error SymbolicName=W3_MSG_READ_RAW_MUST_BE_GLOBAL
Language=English
An attempt was made to load filter '%1' on a server instance but it requires
the SF_NOTIFY_READ_RAW_DATA filter notification so it must be loaded as a global
filter.
.

Messageid=2223 Severity=Informational SymbolicName=W3_MSG_LOADED_FILTER_FROM_REG
Language=English
For compatibility with previous versions of IIS, the filter '%1' was loaded as a global filter from the registry.
To control the filter with the Internet Service Manager, remove the filter from the
registry and add it as a global filter with Internet Service Manager.  Filters in the
registry are stored at "HKLM\System\CurrentControlSet\Services\W3Svc\Parameters\Filter DLLs".
.

Messageid=2226 Severity=Warning SymbolicName=W3_EVENT_CANNOT_READ_FILE_SECURITY
Language=English
The server was unable to read the file %1 due to a lack of access permissions.
.

Messageid=2227 Severity=Error SymbolicName=W3_EVENT_CAL_SSL_EXCEEDED
Language=English
The server was unable to acquire a license for a SSL connection.
.

Messageid=2228 Severity=Warning SymbolicName=W3_EVENT_NO_MORE_CRASH
Language=English
The server stop serving requests for application '%1' because the number of Out of Process
component crashes exceed a limit.
.

Messageid=2229 Severity=Warning SymbolicName=W3_EVENT_FAIL_SHUTDOWN
Language=English
The server failed to shut down application '%1'.  The error was '%2'.
.

Messageid=2230 Severity=Warning SymbolicName=W3_EVENT_CANNOT_READ_FILE_EXIST
Language=English
The server was unable to read the file %1. The file does not exist.
.

Messageid=2231 Severity=Warning SymbolicName=W3_EVENT_CANNOT_READ_FILE
Language=English
The server was unable to read the file %1. The Windows 32 error returned from the attempt is %2.
.

Messageid=2232 Severity=Warning SymbolicName=W3_EVENT_CANNOT_READ_FILE_SIZE
Language=English
The server was unable to read the file %1. The file exceeds the maximum allowable size of %2.
.

Messageid=2233 Severity=Warning SymbolicName=W3_EVENT_CANNOT_READ_FILE_MEMORY
Language=English
The server was unable to allocate a buffer to read the file %1.
.

Messageid=2236 Severity=Warning SymbolicName=W3_EVENT_FAIL_LOADWAM
Language=English
The server failed to load application '%1'.  The error was '%2'.
.

Messageid=2237 Severity=Warning SymbolicName=W3_EVENT_OOPAPP_VANISHED
Language=English
Out of process application '%1' terminated unexpectedly.
.

Messageid=2238 Severity=Error SymbolicName=W3_EVENT_JOB_QUERY_FAILED
Language=English
Job object query failed. The data is the error.
.

Messageid=2239 Severity=Error SymbolicName=W3_EVENT_JOB_SET_LIMIT_FAILED
Language=English
Job object set limit failed. The data is the error.
.

Messageid=2240 Severity=Error SymbolicName=W3_EVENT_JOB_SCEDULE_FAILED
Language=English
Schedule a work item for CPU Accounting or Limits failed. The data is the error.
.

Messageid=2241 Severity=Warning SymbolicName=W3_EVENT_JOB_LOGEVENT_LIMIT
Language=English
Site '%1' hit its CPU Limit. No action was taken.
.

Messageid=2242 Severity=Warning SymbolicName=W3_EVENT_JOB_PRIORITY_LIMIT
Language=English
Site '%1' hit its CPU Limit. The priority of all process on that site has been lowered to idle class.
.

Messageid=2243 Severity=Warning SymbolicName=W3_EVENT_JOB_PROCSTOP_LIMIT
Language=English
Site '%1' hit its CPU Limit. All processes on that site have been terminated.
.

Messageid=2244 Severity=Warning SymbolicName=W3_EVENT_JOB_PAUSE_LIMIT
Language=English
Site '%1' hit its CPU Limit. The site has been paused.
.

Messageid=2245 Severity=Warning SymbolicName=SSL_MSG_TIME_INVALID_SERVER_CERT
Language=English
The server certificate for instance '%1' has expired or is not yet valid.
.

Messageid=2246 Severity=Warning SymbolicName=SSL_MSG_REVOKED_SERVER_CERT
Language=English
The server certificate for instance '%1' has been revoked.
.

Messageid=2247 Severity=Warning SymbolicName=SSL_MSG_UNTRUSTED_SERVER_CERT
Language=English
The server certificate for instance '%1' does not chain up to a trusted root certificate.
.

Messageid=2248 Severity=Warning SymbolicName=SSL_MSG_SIGNATURE_INVALID_SERVER_CERT
Language=English
One of the certificates in the certificate chain of the server certificate for
instance '%1' has an invalid signature.
.

Messageid=2249 Severity=Warning SymbolicName=W3_EVENT_JOB_QUEUE_FAILURE
Language=English
CPU Limits/Accounting failed to queue work item. The data is the error.
.

Messageid=2250 Severity=Warning SymbolicName=SSL_MSG_CERT_MB_ERROR
Language=English
The server certificate for instance '%1' had invalid metabase data associated with it and could not be retrieved; the error encountered was '%2'.
.

Messageid=2251 Severity=Warning SymbolicName=SSL_MSG_CERT_CAPI_ERROR
Language=English
A CryptoAPI error was encountered trying to retrieve the server certificate for instance '%1'; the error encountered was '%2'.
.

Messageid=2252 Severity=Warning SymbolicName=SSL_MSG_CERT_NOT_FOUND
Language=English
The server certificate for instance '%1' could not be retrieved because it could not be found in a certificate store; the error encountered was '%2'
.

Messageid=2253 Severity=Warning SymbolicName=SSL_MSG_CERT_INTERNAL_ERROR
Language=English
The server certificate for instance '%1' could not be retrieved due to an internal error; the error encountered was '%2'.
.

Messageid=2254 Severity=Warning SymbolicName=SSL_MSG_CTL_MB_ERROR
Language=English
The server Certificate Trust List for instance '%1' had invalid metabase data associated with it and could not be retrieved; the error encountered was '%2'.
.

Messageid=2255 Severity=Warning SymbolicName=SSL_MSG_CTL_CAPI_ERROR
Language=English
A CryptoAPI error was encountered trying to retrieve the server Certificate Trust List for instance '%1'; the error encountered was '%2'.
.

Messageid=2256 Severity=Warning SymbolicName=SSL_MSG_CTL_NOT_FOUND
Language=English
The server Certificate Trust List for instance '%1' could not be retrieved because it could not be found in a certificate store; the error encountered was '%2'.
.

Messageid=2257 Severity=Warning SymbolicName=SSL_MSG_CTL_INTERNAL_ERROR
Language=English
The server certificate for instance '%1' could not be retrieved due to an internal error; the error number is '%2'.
.

Messageid=2258 Severity=Warning SymbolicName=W3_EVENT_FAILED_CLOSE_CC_SHUTDOWN
Language=English
The server failed to close client connections to the following URLs during shutdown: 
'%1'.
.

Messageid=2259 Severity=Error SymbolicName=W3_EVENT_FAIL_OOP_ACTIVATION
Language=English
The COM Application '%1' at '%2' failed to activate out of process.
.

Messageid=2261 Severity=Error SymbolicName=W3_MSG_READ_RAW_MUST_USE_STANDARD_APPLICATION_MODE
Language=English
An attempt was made to load filter '%1' but it requires the
SF_NOTIFY_READ_RAW_DATA filter notification and this notification is not
supported in Worker Process Isolation Mode.
.

Messageid=2262 Severity=Warning SymbolicName=W3_EVENT_UNHEALTHY_ISAPI
Language=English
ISAPI '%1' reported itself as unhealthy for the following reason: '%2'.
.

Messageid=2263 Severity=Warning SymbolicName=W3_EVENT_UNHEALTHY_ISAPI_NO_REASON
Language=English
ISAPI '%1' reported itself as unhealthy.  No reason was given by the ISAPI.
.

Messageid=2264 Severity=Warning SymbolicName=W3_EVENT_COMPRESSION_DIRECTORY_INVALID
Language=English
The directory specified for caching compressed content %1 is invalid.  Static compression is being disabled.
.

Messageid=2265 Severity=Warning SymbolicName=W3_EVENT_SUBAUTH_REGISTRY_CONFIGURATION_LOCAL
Language=English
The registry key for IIS subauthenticator is not configured correctly on local machine, the anonymous password sync feature is disabled.
.

Messageid=2266 Severity=Warning SymbolicName=W3_EVENT_SUBAUTH_LOCAL_SYSTEM
Language=English
The account that the current worker process is running under does not have SeTcbPrivilege privilege, the anonymous password sync feature and the Digest authentication feature are disabled.
.

Messageid=2267 Severity=Error SymbolicName=W3_EVENT_RAW_FILTER_CANNOT_BE_STARTED_DUE_TO_HTTPFILTER
Language=English
Raw ISAPI Filter could not be loaded due to configuration problem. HTTPFilter Service has to be hosted in inetinfo.exe for Raw ISAPI filters to work correctly. The most likely reason is that IIS5IsolationModeEnabled setting was changed and HTTPFilter service was not restarted. Restart HTTPFilter for configuration changes to be taken into effect.
.

Messageid=2268 Severity=Error SymbolicName=W3_EVENT_ALL_FILTERS_DID_NOT_LOAD
Language=English
Could not load all ISAPI filters for site/service.  Therefore startup aborted.
.

Messageid=2269 Severity=Error SymbolicName=W3_EVENT_FAIL_INITIALIZING_ULATQ
Language=English
The worker process failed to initialize the http.sys communication or the w3svc communication layer and therefore could not be started.  The data field contains the error number.
.

Messageid=2270 Severity=Error SymbolicName=W3_EVENT_CERTIFICATE_MAPPING_COULD_NOT_BE_LOADED
Language=English
IIS Client certificate mapping configuration for site %1 failed to be loaded. The data field contains the error number.
.

Messageid=2271 Severity=Error SymbolicName=W3_EVENT_LOGGING_MODULE_FAILED_TO_LOAD
Language=English
Could not initialize the logging module for site %1.  The site will therefore be non-functional.
.

Messageid=2272 Severity=Warning SymbolicName=W3_EVENT_SUBAUTH_REGISTRY_CONFIGURATION_DC
Language=English
The registry key for IIS subauthenticator is not configured correctly on domain controller, the Digest feature is disabled.
.

Messageid=2273 Severity=Warning SymbolicName=W3_EVENT_NO_METABASE
Language=English
Worker process could not access metabase due to disconnection error.  Marking process as unhealthy.
.
;
;#endif  // _W3MSG_H_
;
