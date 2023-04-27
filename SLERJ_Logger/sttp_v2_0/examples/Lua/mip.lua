--External Slerj Time Tagged Parser (STTP) Lua script for parsing MicroStrain
--3DM-GX5-25 MIP packet data captured in a SSR-1 time tagged archive.
--This script is executed once when loaded, initializing the parser and
--providing the ParseData function to be called from STTP with data stored
--in each packet of the time tagged archive.

--Use sttp_setTSFormat to set the format string to be provided by STTP.
--This overrides the -N sttp command line argument.  The second boolean
--argument specifies whether milliseconds should be suppressed from the
--timestamp string (equivalent of -S sttp argument).
sttp_setTSFormat("%Y%m%d,%X.",false)

--Use sttp_getPaths to fetch a table of paths that might be useful.  The
--table returned will include fields for the full, absolute path of the
--archive file being parsed (ArchiveFile), the directory of the archive
--file (ArchivePath), the archive file name (ArchiveName), the archive 
--file extension (ArchiveExt), and the current sttp working directory (cwd). 
paths=sttp_getPaths()

--Initialize state for the parser.  The following global variables are
--persistent across calls to ParseData and facilitate continuation of
--parsing on each subsequent call. 
state = 0              --parsing state machine state (see ParseData func)
descriptor = 0         --descriptor of the MIP packet currently being parsed
paylen = 0             --length of the MIP packet being parsed
payload = ""           --payload of the MIP packet as it is being received
RT = 0                 --current RumTime of the SSR-1 during this MIP packet
TS = ""                --TimeStamp string of the SSR-1 during this MIP packet

--Open a file for writing the parsed data.  This file will be created in the
--same folder as the archive file, with the same name, but with _mip.txt
--appended.  The file is opened when the external parser script is loaded
--by sttp. 
local f = assert(io.open(paths.ArchivePath..paths.ArchiveName.."_mip.txt","w"))

--Write a header in the file.
f:write("RunTime,Date,Time,GPSWeek,GPSTOW,Flags,yaw(deg),pitch(deg),roll(deg)\n")

--========================================================================
--                          ParsePayload
--========================================================================
--ParsePayload is a helper function called by ParseData when a complete MIP
--packet has been received and the checksum has been validated.  This function
--works on the MIP descriptor byte and payload provided from ParseData.
--For this example, the ParsePayload function will look for packets matching
--the example set of data (descriptor 0x80, payload with 2 fields: Euler
--angles (descriptor 0x0C) and GPS timestamp (descriptor 0x12).  When that
--packet is received, a line will be written to the output file that includes
--the SSR runtime, SSR timestamp, GPS time info, and Euler angels.
--If any other packet is received, it will be written in ASCII Hex prepended
--with the SSR runtime and timestamp.
local function ParsePayload(descriptor,payload)
  --In this example, we are only interested in Packets with desecriptor 0x80.
  --If anything else is received, do nothing and return.
  if descriptor ~= 0x80 then return end

  --Each payload is made of fields [length, descriptor, data].
  --We're specifically looking for packets that have a pair of fields
  -- 1) CF Euler angles (len=0xE, desc=0x0C, data=roll,pitch,yaw)
  -- 2) GPS Timestamp (len=0xE, desc=0x12, data=TOW,week,flags)
  --If we've received that packet, lets parse it and write an output line
  --that includes the SSR timestamp, GPS timestamp, and Euler angles
  if (payload:byte(1) == 0xE) and (payload:byte(2) == 0x0C) and
     (payload:byte(15) == 0xE) and (payload:byte(16) == 0x12) then
    r,p,y = string.unpack(">fff",payload:sub(3,14))
    TOW,week,flags = string.unpack(">dI2I2",payload:sub(17,28))
    f:write(string.format("%.3f,%s,",RT,TS))
    f:write(string.format("%d,%.3f,%04X,",week,TOW,flags))
    f:write(string.format("%.2f,%.2f,%.2f\n",math.deg(y),math.deg(p),math.deg(r)))  
  else
    --In this case, we did not recognize the packet type we were looking for.
    --So, write the packet descriptor and fields as hex bytes.  Prepend with
    --the timestamp.
    f:write(string.format("%.3f,%s,%02X",RT,TS,descriptor))
    --While there is still payload, iterate through each field writing as hex
    while #payload>0 do
      --each field is [len,descriptor,data].  Get descriptor and the data.
      desc = payload:byte(2)
      data = payload:sub(3,payload:byte(1))

      --write the descriptor for this field followed by the data
      f:write(string.format(" %02X",desc))
      for i=1,#data do
        f:write(string.format("%02X",data:byte(i)))
      end

      --remove this field from the payload string
      payload = payload:sub(payload:byte(1)+1)
    end
    f:write("\n")
  end
end


--========================================================================
--                          CalcPktCksum
--========================================================================
--CalcPktCksum is a helper function called by ParseData to validate the
--MIP packet checksum using the Fletcher checksum method outlined in the
--3DM-GX5-25 users manual.  Given the descriptor and payload of a MIP packet,
--(and assuming the normal MIP packet header), it calculates the two bytes
--of the checksum and returns them to use in validating the received data.
local function CalcPktCksum(descriptor,payload)
  cs1 = 0x75;                  cs2 = 0x75
  cs1 = cs1 + 0x65;            cs2 = cs2 + cs1
  cs1 = cs1 + descriptor;      cs2 = cs2 + cs1
  cs1 = cs1 + #payload;        cs2 = cs2 + cs1
  for i=1,#payload do
    cs1 = cs1 + payload:byte(i)
    cs2 = cs2 + cs1
  end
  return cs1&0xFF, cs2&0xFF
end

--========================================================================
--                          ParseData
--========================================================================
--ParseData is called from STTP for every archive data packet.  It is called
--with the current SSR runtime, timestamp string (formatted as specified on
--the STTP command line with -N, or with the sttp_setTSFormat function in
--this script), and a buffer containing the received data in the archive
--packet. 
function ParseData(runtime, timestamp, buf)
  --Since data is written in ParsePayload, save the SSR clock info in
  --global variables.  Note that the SSR timestamp info captured here is
  --from the current archive packet being parsed by STTP.  Since the SSR
  --time tagged packets are in 2ms intervals, the MIP packet could be split
  --over multiple SSR archive packets.  By capturing the RT and TS here,
  --the timestamp written in ParsePayload above will be the SSR time when
  --the MIP packet checksum was received.  If the time at MIP packet start
  --is preferred, these lines shold be moved into state 1 of the parser
  --below. 
  RT = runtime
  TS = timestamp
  
  --for each byte in the packet, parse looking for MIP packets.
  --This parser is a state based parser.  State is preserved across calls to
  --this function so that subsequent calls can continue where the previous
  --data ended.
  for i = 1,#buf do
    if state == 0 then                  --S0: waiting MIP start char 1 0x75
      if buf:byte(i) == 0x75 then
        state = state + 1
      end
    elseif state == 1 then              --S1: expect MIP start char 2 0x65
      if buf:byte(i) == 0x65 then
        state = state + 1
      else
        state = 0
      end
    elseif state == 2 then              --S2: expect MIP descriptor byte
      descriptor = buf:byte(i)
      state = state + 1
    elseif state == 3 then              --S3: expect MIP length byte
      paylen = buf:byte(i)
      payload = ""
      state = state + 1
    elseif state == 4 then              --S4: expect 'length' payload bytes
      payload = payload .. string.char(buf:byte(i))
      paylen = paylen - 1
      if paylen == 0 then
        state = state + 1
      end
    elseif state == 5 then              --S5: expect MIP checksum byte 1
      rxd_cksum1 = buf:byte(i)
      state = state + 1
    elseif state == 6 then              --S6: expect MIP checksum byte 2
      rxd_cksum2 = buf:byte(i)
      
      --Calculate checksum based on received data and compare to rxd checksum.
      calc_ck1,calc_ck2 = CalcPktCksum(descriptor,payload)
      
      --If the checksum is valid, send the payload of this packet to the
      --payload parser function.
      if (calc_ck1 == rxd_cksum1) and (calc_ck2 == rxd_cksum2) then
        ParsePayload(descriptor,payload)
      end
      state = 0
    end
  end
end    


 