@REM Example of parsing with a custom timestamp.  Note that % characters have
@REM special meaning to the Windows command processor, and have to be entered
@REM in pairs in a batch file.  When using sttp from the command line, single
@REM % would be used instead.

..\..\sttp.exe -n test_out.txt -N "%%m/%%d/%%Y %%H:%%M:%%S." c1214736.dat

@echo Parse complete.
@pause
