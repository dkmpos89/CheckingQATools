@echo OFF
:: cambio del directorio de trabajo para correcto funcionamiento del script
SET DISCO=D:
%DISCO%
cd %~p0

:: Comprobar si un archivo existe
:: RDC
:: TEF
:: BOTEF_ADM_SAVSEG_PP_TEF_2925_V3 
:: TEF
:: Nombre baseline.out: RDC-TEF-BOTEF_ADM_SAVSEG_PP_TEF_2925_V3-TEF.out
:: 
:: Nombre requests.out: COREBANCO-DESARROLLO_SERVICIOS-COREBANCO_CR_TSK_594-FISA

SET PRODUCT_ID=%1
SET PROJECT_ID=%2
SET BRNAME=%3
SET AREA_ID=%4

:: Set Archivo="C:/%PRODUCT_ID%-%PROJECT_ID%-%BRNAME%-%AREA_ID%.out"
SET LOGFILE="D:/modelo_operativo_checking_4.2/logs/%1-%2-%3-%4.out"
:: echo %LOGFILE%

If exist "%LOGFILE%" (
  echo SI
) ELSE (
  echo NO
)