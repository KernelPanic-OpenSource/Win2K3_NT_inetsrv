;/**********************************************************************/
;/**                       Microsoft Windows NT                       **/
;/**                Copyright(c) Microsoft Corp., 1993                **/
;/**********************************************************************/
;
;/*
;    iisrstam.h
;
;    This file is generated by the MC tool from the IISRSTAM.MC message
;    file.
;
;
;    FILE HISTORY:
;        Phillich   6-Oct-1998  Created
;
;*/
;
;
;#ifndef _IISRSTAM_H_
;#define _IISRSTAM_H_
;

SeverityNames=(Success=0x0
               Informational=0x1
               Warning=0x2
               Error=0x3
              )

Messageid=1 Severity=Success SymbolicName=IRSTASTR_USAGE
Language=English
IISRESET.EXE (c) Microsoft Corp. 1998-1999%n%nUsage:%niisreset [computername]%n
    /RESTART            Stop and then restart all Internet services.
    /START              Start all Internet services.
    /STOP               Stop all Internet services.
    /REBOOT             Reboot the computer.
    /REBOOTONERROR      Reboot the computer if an error occurs when starting,
                        stopping, or restarting Internet services.
    /NOFORCE            Do not forcefully terminate Internet services if
                        attempting to stop them gracefully fails.
    /TIMEOUT:val        Specify the timeout value ( in seconds ) to wait for 
                        a successful stop of Internet services. On expiration
                        of this timeout the computer can be rebooted if 
                        the /REBOOTONERROR parameter is specified.
                        The default value is 20s for restart, 60s for stop,
                        and 0s for reboot.
    /STATUS             Display the status of all Internet services.
    /ENABLE             Enable restarting of Internet Services 
                        on the local system.
    /DISABLE            Disable restarting of Internet Services 
                        on the local system.
.
Messageid=2 Severity=Success SymbolicName=IRSTASTR_STATUS_ITEM
Language=English
Status for %1 ( %2 ) : %3
.
Messageid=3 Severity=Success SymbolicName=IRSTASTR_SERVICE_STOPPED
Language=English
Stopped
.
Messageid=4 Severity=Success SymbolicName=IRSTASTR_SERVICE_STOP_PENDING
Language=English
Stop pending
.
Messageid=5 Severity=Success SymbolicName=IRSTASTR_SERVICE_RUNNING
Language=English
Running
.
Messageid=6 Severity=Success SymbolicName=IRSTASTR_SERVICE_CONTINUE_PENDING
Language=English
Continue pending
.
Messageid=7 Severity=Success SymbolicName=IRSTASTR_SERVICE_PAUSE_PENDING
Language=English
Pause pending
.
Messageid=8 Severity=Success SymbolicName=IRSTASTR_SERVICE_PAUSED
Language=English
Paused
.
Messageid=9 Severity=Success SymbolicName=IRSTASTR_SERVICE_DEFAULT
Language=English
Unknown
.
Messageid=10 Severity=Success SymbolicName=IRSTASTR_SERVICE_START_PENDING
Language=English
Start pending
.
Messageid=11 Severity=Success SymbolicName=IRSTASTR_START_SUCCESS
Language=English
Internet services successfully started
.
Messageid=12 Severity=Success SymbolicName=IRSTASTR_STOP_SUCCESS
Language=English
Internet services successfully stopped
.
Messageid=13 Severity=Success SymbolicName=IRSTASTR_REBOOT_SUCCESS
Language=English
Rebooting !
.
Messageid=14 Severity=Success SymbolicName=IRSTASTR_RESTART_SUCCESS
Language=English
Internet services successfully restarted
.
Messageid=15 Severity=Success SymbolicName=IRSTASTR_REBOOT_ON_ERROR
Language=English
Error processing command, attempting to reboot the server.
.
Messageid=16 Severity=Success SymbolicName=IRSTASTR_DISABLED
Language=English
Access to IIS restart API disabled to this computer 
.
Messageid=17 Severity=Success SymbolicName=IRSTASTR_ENABLED
Language=English
Restarting of Internet Services has been enabled.
.
Messageid=18 Severity=Success SymbolicName=IRSTASTR_REMOTE_DISABLED
Language=English
Restarting of Internet Services has been disabled. 
.
Messageid=19 Severity=Success SymbolicName=IRSTASTR_KILL_SUCCESS
Language=English
Internet services successfully killed
.
Messageid=20 Severity=Success SymbolicName=IRSTASTR_KILL_ON_ERROR
Language=English
Attempting kill...
.
Messageid=21 Severity=Success SymbolicName=IRSTASTR_ACCESS_DENIED
Language=English
Access denied, you must be an administrator of the remote computer to use this
command. Either have your account added to the administrator local group of
the remote computer or to the domain administrator global group.
.
Messageid=22 Severity=Success SymbolicName=IRSTASTR_START_FAILED
Language=English;
Start attempt failed.
.
Messageid=23 Severity=Success SymbolicName=IRSTASTR_STOP_FAILED
Language=English;
Stop attempt failed.
.
Messageid=24 Severity=Success SymbolicName=IRSTASTR_RESTART_FAILED
Language=English;
Restart attempt failed.
.
Messageid=25 Severity=Success SymbolicName=IRSTASTR_REBOOT_FAILED
Language=English;
Reboot attempt failed.
.
Messageid=26 Severity=Success SymbolicName=IRSTASTR_KILL_FAILED
Language=English;
Kill attempt failed.
.
Messageid=27 Severity=Success SymbolicName=IRSTASTR_START_ATTEMPT
Language=English
Attempting start...
.
Messageid=28 Severity=Success SymbolicName=IRSTASTR_STOP_ATTEMPT
Language=English
Attempting stop...
.
Messageid=29 Severity=Success SymbolicName=IRSTASTR_REBOOT_ATTEMPT
Language=English
Attempting reboot...
.
Messageid=30 Severity=Success SymbolicName=IRSTASTR_SERVICE_NOT_ACTIVE
Language=English
IIS Admin Service or a service dependent on IIS Admin is not active.  It most likely failed to start, which may mean that it's disabled.
.
;#endif  // _IISRSTAM_H_
;