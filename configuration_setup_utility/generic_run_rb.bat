@rem echo Loading... we have splash for this these days...
@echo on
setlocal
@rem disable any other local rubyopt settings, just in case...
set RUBYOPT=

@rem have 32 bit java available on 64 bit machines. Yikes java, yikes.
set PATH=%WINDIR%\syswow64;%PATH%
@rem add in JAVA_HOME just for fun/in case
set PATH=%PATH%;%JAVA_HOME%\bin
call java -version > NUL 2>&1 || GOTO INSTALL_JAVA

rem success path now

rem parse out java version, which is a mess LOL https://stackoverflow.com/a/49688919/32453
rem Parses x out of 1.x; for example 8 out of java version 1.8.0_xx
rem Otherwise, parses the major version; 9 out of java version 9-ea
set JAVA_VERSION=0
for /f "tokens=3" %%g in ('java -version 2^>^&1 ^| findstr /i "version"') do (
  set JAVA_VERSION=%%g
)
set JAVA_VERSION=%JAVA_VERSION:"=%
for /f "delims=.-_ tokens=1-2" %%v in ("%JAVA_VERSION%") do (
  if /I "%%v" EQU "1" (
    set JAVA_VERSION=%%w
  ) else (
    set JAVA_VERSION=%%v
  )
)

rem newer versions need help to actually send signals to child processes?
if %JAVA_VERSION% LSS 9 (
     java  -splash:audio.jpg -jar jruby-complete-9.2.13.0.jar -I. %*
   ) else (
     java  --add-opens java.base/sun.nio.ch=ALL-UNNAMED --add-opens java.base/java.io=ALL-UNNAMED -splash:audio.jpg -jar jruby-complete-9.2.13.0.jar -I. %*
   )

GOTO DONE

:INSTALL_JAVA
msg "%username%" you need to install java first please install it from java.com then run again
start http://java.com

:DONE
