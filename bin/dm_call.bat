ECHO off
SETLOCAL

::Parametros
SET all_params=%*
SET producto=COREBANCO
SHIFT
SET proyecto=SERVICIOS_PLANES_AFINIDADES
SHIFT
SET requestId=COREBANCO_CR_TSK_534
SHIFT
SET attrAplicativo=SPA


START E:\Programas\PaExec\paexec.exe \\192.168.10.63 -u checking -p chm.321. -d D:\modelo_operativo_checking_4.2\certificacion_request.bat %producto% %proyecto% %requestId% %attrAplicativo% 
EXIT /B 0