if not "%1"=="" goto unattended
sysocmgr.exe /i:%0\..\sysoc.inf /c /n 
goto finish

:unattended
sysocmgr.exe /i:%0\..\sysoc.inf /c /n /u:%1

:finish

