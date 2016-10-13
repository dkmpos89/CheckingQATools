ECHO off
SETLOCAL

:: Configuramos la rutas de los archivos de registros
SET rutalog=D:\logs_serena\dllaction\dm_CALL.log
SET rutalogPsExec=D:\logs_serena\PsExec\PsExec.log
SET rutasql=D:\procesos_serena\paso
SET mensajerror=

:: Escribimos el inicio del log
CALL :escribirlog "%rutalog%" "-------------------- ENTRADA -----------------------"
CALL :escribirlog "%rutalog%" "PARAMETROS: (%*)"

::Parametros
SET all_params=%*
SET producto=%1
SHIFT
rem El valor de proyecto no se esta considerando
SET proyecto=%1
SHIFT
SET requestTipo=%1
SHIFT
SET requestId=%1
SHIFT
SET estadoFrom=%1
SHIFT
SET estadoTo=%1
SHIFT
SET attrAplicativo=%1

rem IF /I NOT "%requestTipo%" == "CR_TSK" GOTO finalizar
IF /I NOT "%estadoFrom%" == "EN_TRABAJO" (
	CALL :escribirlog "%rutalog%" "El estadoFrom tiene valor en %estadoFrom% no se ejecuta el proceso"
	GOTO finalizar
)
IF /I NOT "%estadoTo%" == "CHECKING_QA"  (
	CALL :escribirlog "%rutalog%" "El estadoTo tiene valor en %estadoTo% no se ejecuta el proceso"
	GOTO finalizar
)
rem El campo proyecto no es el correcto. Solucion: Se obtiene de la misma manera que lo hacìa dm_action.bat

:: Generamos el SQL segun los parametros 
ECHO SET heading OFF > %rutasql%\query%requestId%.sql
ECHO SET feedback OFF >> %rutasql%\query%requestId%.sql
ECHO SET ECHO OFF >> %rutasql%\query%requestId%.sql
ECHO SELECT DISTINCT pwi.workSET_name >> %rutasql%\query%requestId%.sql
ECHO FROM dimensions_adm.pcms_chdoc_data pcd, dimensions_adm.pcms_chdoc_related_workSETs pcrw, dimensions_adm.pcms_workSET_info pwi >> %rutasql%\query%requestId%.sql
ECHO WHERE pcd.ch_doc_id = '%requestId%' >> %rutasql%\query%requestId%.sql
ECHO AND pcd.ch_uid = pcrw.from_ch_uid >> %rutasql%\query%requestId%.sql
ECHO AND pcrw.to_workSET_uid = pwi.workSET_uid >> %rutasql%\query%requestId%.sql

:: Ejecutamos el SQL generado
dmdba pcms_sys/PcMsBcor14@dim12 @%rutasql%\query%requestId%.sql > %rutasql%\salida%requestId%.txt

FOR /f "skip=3" %%s in (%rutasql%\salida%requestId%.txt) do (
  SET proyecto=%%s
  GOTO:proy3:)
:proy3:

CALL :escribirlog "%rutalog%" "Avisar a Checkin QA: Producto:%producto% Proyecto:%proyecto% Request:%requestId% Aplicativo:%attrAplicativo%"

:: Ejecutamos el Proceso en el servidor de checking
CALL :ejecutarproceso

:finalizar

::Escribimos el fin del log
CALL :escribirlog "%rutalog%" "%mensajerror%"
CALL :escribirlog "%rutalog%" "-------------------- SALIDA  -----------------------"

:: Forzamos la salida
EXIT /B %ERRORLEVEL%

:: -------------------------------------------------
:: Funcion para escribir en el log
:: Parametros: ArchivoLog Mensaje
:: -------------------------------------------------
:escribirlog
	::Calculamos la fecha hora
	CALL :obtenerfechahora 
	::Escribimos en el LOG
	ECHO %fechahora% %2 >> "%1%"
	::Escribimos en la consola
	ECHO %fechahora% %2
	::Salimos  del la subrutina
	EXIT /B 0

:: -------------------------------------------------
:: Funcion para obtener la fecha hora
:: Parametros: Ninguno
:: -------------------------------------------------
:obtenerfechahora
	:: Inicio de configuracion de fecha
	IF "%LANG%" == "ES" (
	  SET FechaExec=%date:~6,4%-%date:~3,2%-%date:~0,2%
	) ELSE (
	  SET FechaExec=%date:~6,4%-%date:~0,2%-%date:~3,2%
	)
	SET FechaExec=%FechaExec: =0%
	SET hour=%time:~0,2%
	IF "%hour:~0,1%" == " " SET hour=0%hour:~1,1%
	SET min=%time:~3,2%
	IF "%min:~0,1%" == " " SET min=0%min:~1,1%
	SET secs=%time:~6,2%
	IF "%secs:~0,1%" == " " SET secs=0%secs:~1,1%
	SET mili=%time:~9,2%
	IF "%mili:~0,1%" == " " SET mili=0%mili:~1,1%
	::Configuramos la fechahora
	SET fechahora=%FechaExec% %hour%:%min%:%secs%:%mili%
	::Salimos  del la subrutina
	EXIT /B 0
	
:: -------------------------------------------------
:: Definimos la función con timeout que ejecuta el proceso remoto en el servidor del checking
:: -------------------------------------------------
:ejecutarproceso

	::Configuramos el nombre del proceso
	SET NombreExe=paexec.exe

	::Configuramos el tiempo maximo a esperar (SEGUNDOS)
	SET SEGUNDOSMAXIMOS=20

	::Configuramos el el maximo de reintentos
	SET REINTENTOSMAXIMOS=3

	::Inicializamos el contador de segundos
	SET CONTADORSEGUNDOS=0
	
	::Inicializamos el contador de intentos
	SET CONTADORINTENTOS=0

	::Log ENTRADA
	CALL :escribirlog "%rutalogPsExec%" "-------------------- ENTRADA -----------------------"
	CALL :escribirlog "%rutalogPsExec%" "PARAMETROS: (%all_params%)"
	
	:INICIARPROCESO
	::Inicializamos el contador de segundos
	SET CONTADORSEGUNDOS=1
	::Contamos el intentos
	SET /A CONTADORINTENTOS+=1
	::Log de Intento
	CALL :escribirlog "%rutalogPsExec%" "Intento #%CONTADORINTENTOS%"
	::Retardo
	TIMEOUT /T 1 > NUL
	::Iniciamos el Programa
	::START c:\Windows\System32\calc.exe
	::START D:\procesos_serena\PSTools\%NombreExe% \\192.168.10.63 -u checking -p chm.321. -d D:\modelo_operativo_checking_4.2\certIFicacion_request.bat %producto% %proyecto% %requestId% %attrAplicativo% >> "%rutalogPsExec%" 2>&1
	START D:\procesos_serena\PSTools\%NombreExe% \\192.168.10.63 -u checking -p chm.321. -d -lo "%rutalogPsExec%" D:\modelo_operativo_checking_4.2\certIFicacion_request.bat %producto% %proyecto% %requestId% %attrAplicativo% 
    GOTO ESPERAR
	
	:ESPERAR
	::CALL :escribirlog "%rutalogPsExec%" "Debug CONTADORSEGUNDOS:%CONTADORSEGUNDOS%, SEGUNDOSMAXIMOS:%SEGUNDOSMAXIMOS%, CONTADORINTENTOS:%CONTADORINTENTOS% "
	IF %CONTADORSEGUNDOS% GEQ %SEGUNDOSMAXIMOS% GOTO MATARPROCESO
	IF %CONTADORINTENTOS% GTR %REINTENTOSMAXIMOS% GOTO FALLIDO
	TIMEOUT /T 1 > NUL
	SET /A CONTADORSEGUNDOS+=1
	FOR /F "delims=" %%a IN ('TASKLIST ^| FIND /C "%NombreExe%"') DO IF %%a EQU 0 GOTO CORRECTO
	GOTO ESPERAR

	:MATARPROCESO
	::Matamos el Proceso
	CALL :escribirlog "%rutalogPsExec%" "Matando Proceso %NombreExe%"
	TASKKILL /IM %NombreExe% /F > NUL
	SET mensajerror=Proceso matado exitosamente. (%CONTADORSEGUNDOS% seg.)
	CALL :escribirlog "%rutalogPsExec%" "%mensajerror%"
	GOTO INICIARPROCESO
	
	:CORRECTO
	SET mensajerror=Proceso ejecutado correctamente en el #%CONTADORINTENTOS% intento. (%CONTADORSEGUNDOS% seg.)
	CALL :escribirlog "%rutalogPsExec%" "%mensajerror%"
	GOTO TODOBIEN
	
	:FALLIDO
	SET mensajerror=Proceso fallido en %REINTENTOSMAXIMOS% reintentos. (%CONTADORSEGUNDOS% seg.)
	CALL :escribirlog "%rutalogPsExec%" "%mensajerror%"
	GOTO TODOBIEN	
		
	:TODOBIEN	
	::Log SALIDA
	CALL :escribirlog "%rutalogPsExec%" "-------------------- SALIDA  -----------------------"
	::Salimos  del la subrutina
	EXIT /B 0