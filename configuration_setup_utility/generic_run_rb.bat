@rem echo Loading... we have splash for this these days...

@setlocal
@rem disable any local rubyopt settings, just in case...
@set RUBYOPT=

@rem run using bundled Java 8 JRE
@set PATH=%~dp0vendor\jre1.8.0_251\bin
@call java -version > NUL 2>&1 || GOTO INSTALL_JAVA

@rem success path
@java -splash:audio.jpg -jar jruby-complete-1.7.12.jar --1.9 -I. %*

@GOTO DONE

:INSTALL_JAVA
msg "%username%" you need to install java first please install it from java.com then run again
start http://java.com

:DONE
