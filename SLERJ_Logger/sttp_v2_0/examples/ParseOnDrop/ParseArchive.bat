pushd %~dp1

@REM - Add the path to STTP, or just use the full path on the 
@REM   line that executes it.
PATH=%PATH%;j:\sttp\


sttp.exe -n %~n1_out.txt -S -N "%%Y/%%m/%%d %%H:%%M " %1

@REM - remove the line below if you don't want the window to wait for the user.
pause
