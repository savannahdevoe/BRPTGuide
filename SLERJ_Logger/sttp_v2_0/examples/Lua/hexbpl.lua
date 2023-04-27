--External Slerj Time Tagged Parser (STTP) Lua script for parsing archive
--data into an output file with a single ASCII hex byte per line.  This
--replicates the -d <outfile> --dat-bpl options of STTP as a demonstration
--of the external Lua script capabilities.
--This script is executed once when loaded, initializing the parser and
--providing the ParseData function to be called from STTP with data stored
--in each packet of the time tagged archive.

--Unlike -d <outfile> --dat-bpl, we will also include a formatted timestamp
--based on the RTC, as if this were timestamped line output.  Also, unlike
--the -d sttp output, we will write the runtime in decimal seconds instead
--of milliseconds.
--Use sttp_setTSFormat to set the format string to be provided by STTP.
--This overrides the -N sttp command line argument.  The second boolean
--argument specifies whether milliseconds should be suppressed from the
--timestamp string (equivalent of -S sttp argument).
sttp_setTSFormat("%Y%m%d %H%M%S.",false)

--Use sttp_getPaths to fetch a table of paths that might be useful.  The
--table returned will include fields for the full, absolute path of the
--archive file being parsed (ArchiveFile), the directory of the archive
--file (ArchivePath), the archive file name (ArchiveName), the archive 
--file extension (ArchiveExt), and the current sttp working directory (cwd). 
paths=sttp_getPaths()

--Open a file for writing the parsed data.  This file will be created in the
--same folder as the archive file, with the same name, but with _out.txt
--appended.  The file is opened when the external parser script is loaded
--by sttp. 
local f = assert(io.open(paths.ArchivePath..paths.ArchiveName.."_bpl.txt","w"))

--========================================================================
--                          ParseData
--========================================================================
--ParseData is called from STTP for every archive data packet.  It is called
--with the current SSR runtime, timestamp string (formatted as specified on
--the STTP command line with -N, or with the sttp_setTSFormat function in
--this script), and a buffer containing the received data in the archive
--packet. 
function ParseData(runtime, timestamp, buf)
  --Write a line with the current archive packet timestamp for each byte
  --in the packet.
  for i = 1,#buf do
    f:write(string.format("%.3f %s %02X\n",runtime, timestamp, buf:byte(i)))
  end
end    


 