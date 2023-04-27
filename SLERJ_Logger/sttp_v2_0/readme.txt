Version 2.0 - 2 Mar 2019 (see changelog below)

The SLERJ Time Tagged Parser is a Windows command line utility (sttp.exe) 
provided with the SSR loggers to parse time tagged archives into various
output types.  Given a time tagged archive, the utility can produce a
variety of output files including:
  => the original raw stream (with no timing information),
  => timestamped text lines (for archives of recorded text data),
  => and detailed contents of the archive to include:
     - a time correlation packet file,
     - a data packet file that has a line for each data frame,
     - and a mixed file that interlaces time correlation with data packets.

***** NEW WITH VERSION 2.0 *****
Version 2 of STTP now supports user defined scripts to parse payload data
with reference to the SSR timestamps embedded in time tagged archives.
Check out the Lua scripts in the examples subfolder.

For timestamped-line output, the format of the time stamp can be specified
using any of the identifiers used by the standard C89 library function strftime.
A list is provided below for reference.

For the detailed archive content output (options -t, -d, and -m), data bytes 
are represented as a series of hexadecimal text characters. An example of each 
of the output files is below.   Usage of the sttp utility is summarized by 
its help output:

usage: sttp.exe [options] <infile>
    Version 2.0, Mar  2 2019 11:05:21
    options:
      -h                Include headers in tcp and dat files.
      -r <raw_file>     Write raw stream data to raw_file
      -x <lua_file>     Send data to an external lua script for parsing
      -t <tcp_file>     Write Time Correlation Packets to tcp_file
      -d <dat_file>     Write Tagged data to dat_file
      -m <mxd_file>     Write both TCPs and tagged data to mxd_file
      -n <txt_file>     Write timestamped line text file
      -N "string"       Date format string for tagged line output (strftime)
      -S                Suppress milliseconds in tagged line output
      --nointerp        Use pre-v1.6 behavior, not interpolating timestamps in -n output
      --dat-bpl         Modified Tagged data (-d) output to give one data byte per line
    Interval extraction options for timestamped line output:
         For the arguments below, 'N' is assumed to be in seconds unless suffixed with 'L', which
         denotes lines.  For example '-i 30' denotes an interval of 30 seconds, where '-i 30L' denotes
         an interval of 30 timestamped lines of data.
      -k,--skip N       Skip N seconds/lines of data before producing output
      -i,--interval N   Extract excerpts at intervals of N seconds/lines
      -w,--window N     Extract N seconds/lines at each interval
      -v,--nwins M      Process M windows (default=0, which means to end of file)


Timestamped-line text output:
  Note that for timestamped-line output, each line that contains data will be
  prepended by a date string as determined by the specified format (or the
  default format "%Y %m %d %H %M %S "), followed by the three digit millisecond
  portion of the timestamp, followed by the contents of the line.  The time
  stamp prepended to the line corresponds to the time of the 2ms cycle in
  which the first character of the line was received.

  Consider an example in which the following text output is recorded:
    S D  0.0000122 kg
    S D  0.0000122 kg
    S D  0.0000122 kg
    S D  0.0000123 kg

  Timestamped-line text output example using the default format:
    2014 02 03 21 47 38 915 S D  0.0000122 kg
    2014 02 03 21 47 39 013 S D  0.0000122 kg
    2014 02 03 21 47 39 111 S D  0.0000122 kg
    2014 02 03 21 47 39 207 S D  0.0000123 kg

  Timestamped-line text output example using -N "%m/%d/%Y %H:%M:%S.":
    02/03/2014 21:47:38.915 S D  0.0000122 kg
    02/03/2014 21:47:39.013 S D  0.0000122 kg
    02/03/2014 21:47:39.111 S D  0.0000122 kg
    02/03/2014 21:47:39.207 S D  0.0000123 kg


Time Correlation Packet output example:
    RunTime(ms) Year Month Day Hour Minute Second
    4196 2013 3 25 9 52 4.625
    604196 2013 3 25 10 2 3.628
    1204196 2013 3 25 10 12 2.486

Tagged Data output example:
    RunTime(ms) count HexBytes
    4196 20 322E323530333630652B303520322E3339343433
    4198 23 30652D3034202D312E343530303639652D303420322E37
    4200 23 3637343235652D303420312E373134373036652D303120

Mixed output example:
    A3 4196 2013 3 25 9 52 4.625
    A2 4196 20 322E323530333630652B303520322E3339343433
    A2 4198 23 30652D3034202D312E343530303639652D303420322E37
    …
    A2 604194 23 3032202D352E353633313634652D303120312E32323636
    A3 604196 2013 3 25 10 2 3.628
    A2 604196 23 3330652D303220332E313334343333652B303020302037


Timestamped-line mode format options
  For timestamped-line mode, the specific form of the timstamp may be specified
  on the command line with the -N option.  For example, to add a timestamp of
  the form yyyy-mm-dd hh:mm:ss.msec, add to the command line:
     -N "%Y-%m-%d %H:%M:%S."
  The % fields are the same as those defined for the standard C library
  function strftime.  The most common are listed below.  Unless -S is specified
  on the sttp command line to suppress the milliseconds field, the three digit 
  value of milliseconds will immediately follow the result of the format string.  
  In the example above, the period at the end of the string serves as the  
  decimal in the fractional seconds field that results.  An example of lines  
  generated using this format string and no -S would be:
      2014-02-03 21:47:38.915 S D  0.0000122 kg
      2014-02-03 21:47:39.013 S D  0.0000122 kg
      2014-02-03 21:47:39.111 S D  0.0000122 kg
      2014-02-03 21:47:39.207 S D  0.0000123 kg

  Common timestamp format fields:
   Field Example
    %b   Aug                       Abbreviated month name
    %B   August                    Full month name
    %c   Thu Aug 23 14:55:02 2001  Date and time representation
    %d   23                        Day of the month, zero-padded (01-31)
    %H   14                        Hour in 24h format (00-23)
    %I   02                        Hour in 12h format (01-12)
    %j   235                       Day of the year (001-366)
    %m   08                        Month as a decimal number (01-12)
    %M   55                        Minute (00-59)
    %p   PM                        AM or PM designation
    %S   02                        Second (00-61)
    %x   08/23/01                  Date representation
    %X   14:55:02                  Time representation
    %y   01                        Year, last two digits (00-99)
    %Y   2001                      Year
    %%                             A % sign  %

In version 2, the -x option has been added to send the packet data to a Lua
script for parsing.  The script must define the function:
  ParseData(runtime,timestamp,data)
The lua script is executed when it is loaded, allowing parser state to be 
initialized and output file(s) to be opened as required.  Then the ParseData
function will be called for each time tagged data packet in the archive, and
will receive the packet runtime (in msec), a formatted timestamp based on the
SSR real time clock using the provided strftime format string, and the bytes
stored in the time tagged data packet.

The Lua Environemnt provides a couple of additional functions that can
reach back into STTP.
  sttp_setTSFormat(string fmt, bool SuppressMSec)
      This function allows the timestamp format string to be provided in
      the lua script instead of on the sttp command line using -N.  The
      second argument replaces the command line argument -S.  This function
      overrides the command line arguments.
  sttp_getPaths()
      This function returns a table with useful path information.  Fields
      of the table are:
      ArchiveFile - the full, absolute path of the archive file being parsed
      ArchivePath - the directory of the archive file
      ArchiveName - the name of the archive file (without path or extension)
      ArchiveExt  - the archive file extension
      cwd         - the STTP current working directory.

See the Lua examples for details on how to use these. 


==============================================================================
ChangeLog
---------
Version 2.0 - 2 Mar 2019
  Added Lua script engine to support external, application specific data
  parsers.  This allows binary data stored in a time tagged archive to be
  parsed with the SSR clock information available for reference.  See the
  lua examples for detials.

Version 1.7 - 7 Feb 2019
  Added an option --dat-bpl to change the -d (dat file) output to give
  one byte per line so that each line has only RunTime and a single byte.
  
Version 1.6 - 15 Mar 2018
  Reworked timestamped-line output to interpolate the RTC based timestamp to 
  compensate for drift of the free running msec clock used to tag data packets.

  Added the option --nointerp to revert to the original behavior. 
  
Version 1.5 - 21 Feb 2018
  Added support for time tagged archives recorded at high data rates and
  bandwidth (>635 kbaud).  This corresponds to a fix in SSR-1 firmware
  v1.2.1.

Version 1.4 - 10 Aug 2017
  Added options to decimate timestamp line output (only output at intervals).
  
Version 1.3 - 4 Feb 2017
  Corrected bug that caused high baud recordings (>315k baud) to fail parsing.
  
Version 1.2 - 10 May 2016
  Corrected bug where -n option was looking for 0xC as newline instead of 0xD
  
Version 1.1 - 19 Feb 2015
  Added timestamped-line mode for text data recorded in an archive.
    