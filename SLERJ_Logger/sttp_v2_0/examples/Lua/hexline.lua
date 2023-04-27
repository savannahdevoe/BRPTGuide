--External Slerj Time Tagged Parser (STTP) Lua script for parsing archive
--data into an output file with a single ASCII hex byte per line.  This
--replicates the -d <outfile> option of STTP as a demonstration
--of the external Lua script capabilities.
--This script is executed once when loaded, initializing the parser and
--providing the ParseData function to be called from STTP with data stored
--in each packet of the time tagged archive.

--Use sttp_getPaths to fetch a table of paths that might be useful.  The
--table returned will include fields for the full, absolute path of the
--archive file being parsed (ArchiveFile), the directory of the archive
--file (ArchivePath), the archive file name (ArchiveName), the archive 
--file extension (ArchiveExt), and the current sttp working directory (cwd). 
paths=sttp_getPaths()

--Open a file for writing the parsed data.  This file will be created in the
--same folder as the archive file, with the same name, but with _hexline.txt
--appended.  The file is opened when the external parser script is loaded
--by sttp. 
local f = assert(io.open(paths.ArchivePath..paths.ArchiveName.."_hexline.txt","w"))

--========================================================================
--                          ParseData
--========================================================================
--ParseData is called from STTP for every archive data packet.  It is called
--with the current SSR runtime, timestamp string (formatted as specified on
--the STTP command line with -N, or with the sttp_setTSFormat function in
--this script), and a buffer containing the received data in the archive
--packet. 
function ParseData(runtime, timestamp, buf)
  --The -d option of STTP creates lines with runtime in milliseconds, the 
  --packet data byte count, and then the data bytes in ASCII-Hex.
  f:write(string.format("%.0f %d ",runtime*1000, #buf))
  for i = 1,#buf do
    f:write(string.format("%02X",buf:byte(i)))
  end
  f:write("\n");
      
end    


 