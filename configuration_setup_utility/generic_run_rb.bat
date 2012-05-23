@rem echo Loading... we have splash for this now

@setlocal
@rem disable any local rubyopt settings, just in case...
@set RUBYOPT=

@rem have 32 bit java available on 64 bit machines. Yikes java, yikes.
@set PATH=%WINDIR%\syswow64;%PATH%
@rem add in JAVA_HOME just for fun/in case
@set PATH=%PATH%;%JAVA_HOME%\bin
@call java -version > NUL 2>&1 || echo you need to install java JRE first please install it from java.com then run again && java -version && pause && GOTO INSTALL_JAVA

@rem success path
@java -splash:audio.jpg -jar jruby-complete-1.6.4.jar %*

@GOTO DONE

:INSTALL_JAVA
start http://java.com

:DONE