@REM Example of parsing with a custom timestamp.  Note that % characters have
@REM special meaning to the Windows command processor, and have to be entered
@REM in pairs in a batch file.  When using sttp from the command line, single
@REM % would be used instead.

@REM pull one line out from every 60 seconds of recorded data
..\..\sttp.exe -n test_out_60_1L.txt -i 60 -w 1L st150237.dat

@REM pull one second of data from every 60 seconds of recorded data
..\..\sttp.exe -n test_out_60_1.txt -i 60 -w 1 st150237.dat

@REM pull two seconds of data from every 30 seconds of recorded data
@REM after skipping the first 300 seconds of the file.  Also, only pull
@REM out 10 intervals of data.
..\..\sttp.exe -n test_out_30_2.txt -k 300 -i 30 -w 2 --nwins 10 st150237.dat


@echo Parse complete.
@pause
