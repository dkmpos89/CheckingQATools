@echo OFF
:: cambio del directorio de trabajo para correcto funcionamiento del script
SET DISCO=D:
%DISCO%
cd %~p0

SET PRODUCT_ID=%1
SET PROJECT_ID=%2
SET BRNAME=%3
SET AREA_ID=%4

CALL:MAIN
goto:EOF

:MAIN
:: Configuramos el tiempo maximo a esperar (600 SEGUNDOS = 10 minutos)
SET SEGUNDOSMAXIMOS=600
:: Inicializamos el contador de segundos
SET CONTADORSEGUNDOS=0
:: SET LOGFILE="C:/%PRODUCT_ID%-%PROJECT_ID%-%BRNAME%-%AREA_ID%.out"
SET LOGFILE="D:/modelo_operativo_checking_4.2/logs/%PRODUCT_ID%-%PROJECT_ID%-%BRNAME%-%AREA_ID%.out"
SET ERRFILE="D:/modelo_operativo_checking_4.2/logs/%PRODUCT_ID%-%PROJECT_ID%-%BRNAME%-%AREA_ID%.err"
goto ESPERAR
	

:ESPERAR
:: Se espera un minuto y luego se consulta si existe el fichero de log .out
if %CONTADORSEGUNDOS% GEQ %SEGUNDOSMAXIMOS% goto ERROR
waitfor SomethingThatIsNeverHappening /t 30 2>NUL
SET /A CONTADORSEGUNDOS+=30
:: echo %LOGFILE%
If exist "%LOGFILE%" ( goto FINALIZAR ) ELSE ( goto ESPERAR )

:ERROR
:: Se informa que se ha exedido el tiempo maximo de espera sin encontrar el archivo .out
ECHO ERR_TIMEOUT
::goto MAIN
EXIT /b 0

:FINALIZAR
:: Saliendo...
ECHO READYOUT
If exist "%ERRFILE%" ( echo FAILOUT ) 
EXIT /b 0

goto:EOF