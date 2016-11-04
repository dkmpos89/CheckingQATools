@ECHO off
cd %~p0

SET PRODUCT_ID=%1
SET PROJECT_ID=%2
SET BRNAME=%3
SET AREA_ID=%4

CALL:MAIN
GOTO:EOF

:MAIN
:: Configuramos el tiempo maximo a esperar (600 SEGUNDOS = 10 minutos)
SET SEGUNDOSMAXIMOS=600
:: Inicializamos el contador de segundos
SET CONTADORSEGUNDOS=0
:: SET LOGFILE="C:/%PRODUCT_ID%-%PROJECT_ID%-%BRNAME%-%AREA_ID%.out"
SET LOGFILE="D:/modelo_operativo_checking_4.2/logs/%1-%2-%3-%4.out"
GOTO ESPERAR
	

:ESPERAR
:: Se espera un minuto y luego se consulta si existe el fichero de log .out
IF %CONTADORSEGUNDOS% GEQ %SEGUNDOSMAXIMOS% GOTO ERROR
TIMEOUT /T 30 > NUL
SET /A CONTADORSEGUNDOS+=30
ECHO %CONTADORSEGUNDOS%
echo %LOGFILE%
If EXIST %LOGFILE% ( GOTO FINALIZAR ) ELSE ( GOTO ESPERAR )


:ERROR
:: Se informa que se ha exedido el tiempo maximo de espera sin encontrar el archivo .out
ECHO "Tiempo maximo de espera agotado!"
::GOTO MAIN
GOTO:EOF

:FINALIZAR
:: Saliendo...
ECHO READYOUT
GOTO:EOF
