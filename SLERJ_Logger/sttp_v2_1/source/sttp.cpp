/*
Copyright (c) 2019 Slerj, LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#define __STTP_VERSION__ "2.1"

/*
Version 2.1 - 26 Apr 2019
  Fixed exception on exit if Lua script isn't used.
  
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
*/

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <conio.h>
#include <time.h>
#include "anyoption.h"
#include "lua.hpp"

//Causes the program to exit with the given return value.
//Does a printf to stderr with the given arguments.  
void ExitError(int retval, const char *fmt, ...);

//Given a path, return just the filename and extension.
std::string BaseFileName(const char *path);

//Structure to hold Time Correlation Packet contents
typedef struct
  {
  unsigned long RT;           //SSR Runtime in msec
  unsigned short year;        //year  1-4095
  unsigned short month;       //month 1-12
  unsigned short day;         //day   1-31
  unsigned short hour;        //hour  0-23
  unsigned short min;         //min   0-59
  unsigned short sec;         //sec   0-59
  unsigned short msec;        //msec  0-999
  } tTCP;

//Packet fetch, parse, and validation routines.
std::string GetPacket(FILE *fpi, unsigned long *loc);
bool ValidateCksum(std::string s);
tTCP ParseTCP(std::string pkt);
tTCP GetNextTCP(FILE *fp);

//Functions to calculate times and create text timestamps
tTCP GetSubPacketTime(tTCP &PrevTCP, tTCP &NextTCP, unsigned long RT_sec, unsigned short msec);
std::string GetLineTimeStamp(tTCP &tcp, std::string &format, bool SuppressMSec);
unsigned long ScaledDT(unsigned long RT, tTCP &PrevTCP, tTCP &NextTCP);
unsigned long GetTCPDiff_msec(tTCP &newer, tTCP &older);

bool IntervalSetup(std::string skip, std::string interval, std::string window, std::string nwins);
bool IntervalWriteEnabled(tTCP &tcp, unsigned long TSLinesGenerated);

//Convience function for writing data and mixed file information
void WriteDMLine(FILE *fpd, FILE *fpm, const char *fmt, ...);

//Functions to implement external Lua Script parser support
lua_State *LuaSetup(char *fname);
void LuaParse(lua_State *L, std::string &subpkt, double RT, std::string &timestamp);

//Global Data referenced by Lua environment
std::string TFormat;        //format string for custom timestamp generation
bool SuppressMSec;
std::string ArchFilePath;   //Complete path to the archive file being parsed

//========================================================================
//                             main
//========================================================================
int main(int argc, char *argv[])
  {
  //----------------------- command line processing ----------------------
  //command line processing
  AnyOption *opt = new AnyOption();
  opt->addUsage("usage: %s [options] <infile>\n", BaseFileName(argv[0]).c_str());
  opt->addUsage("    Version " __STTP_VERSION__ ", " __DATE__ " " __TIME__ "\n");
  opt->addUsage("    options:\n");
  opt->addUsage("      -h                Include headers in tcp and dat files.\n");
  opt->addUsage("      -r <raw_file>     Write raw stream data to raw_file\n");
  opt->addUsage("      -x <lua_file>     Send data to an external lua script for parsing\n");
  opt->addUsage("      -t <tcp_file>     Write Time Correlation Packets to tcp_file\n");
  opt->addUsage("      -d <dat_file>     Write Tagged data to dat_file\n");
  opt->addUsage("      -m <mxd_file>     Write both TCPs and tagged data to mxd_file\n");
  opt->addUsage("      -n <txt_file>     Write timestamped line text file\n");
  opt->addUsage("      -N \"string\"       Date format string for tagged line output (strftime)\n");
  opt->addUsage("      -S                Suppress milliseconds in tagged line output\n");
  opt->addUsage("      --nointerp        Use pre-v1.6 behavior, not interpolating timestamps in -n output\n");
  opt->addUsage("      --dat-bpl         Modified Tagged data (-d) output to give one data byte per line\n");
  opt->addUsage("    Interval extraction options for timestamped line output:\n");
  opt->addUsage("         For the arguments below, 'N' is assumed to be in seconds unless suffixed with 'L', which\n");
  opt->addUsage("         denotes lines.  For example '-i 30' denotes an interval of 30 seconds, where '-i 30L' denotes\n");
  opt->addUsage("         an interval of 30 timestamped lines of data.\n");
  opt->addUsage("      -k,--skip N       Skip N seconds/lines of data before producing output\n");
  opt->addUsage("      -i,--interval N   Extract excerpts at intervals of N seconds/lines\n");
  opt->addUsage("      -w,--window N     Extract N seconds/lines at each interval\n");
  opt->addUsage("      -v,--nwins M      Process M windows (default=0, which means to end of file)\n");
  
  opt->autoUsagePrint(true);
  opt->setVerbose();
  
  opt->setOption('r');
  opt->setOption('x');
  opt->setOption('t');
  opt->setOption('d');
  opt->setOption('m');
  opt->setOption('n');
  opt->setOption('N');
  opt->setFlag('S');
  opt->setFlag('O');    //include packet offset (archive file bytes) in t,d,and m files.
  opt->setFlag('h');
  opt->setFlag("nointerp");
  opt->setFlag("dat-bpl");
  opt->setOption("skip",'k');
  opt->setOption("interval",'i');
  opt->setOption("window",'w');
  opt->setOption("nwins",'v');
  opt->processCommandArgs(argc,argv);

  //validate required arguments.  At a minumum, the input file must be
  //specified.
	if((opt->getArgc()!=1))
    {
    opt->printUsage();
    delete opt;
		return(0);
    }
  
  //Get interval options and perform setup.  If there is an error parsing
  //the provided values, better to happen before we start opening files.
  char *c;
  c = opt->getValue('k');     std::string skip = c?c:"";
  c = opt->getValue('i');     std::string interval = c?c:"";
  c = opt->getValue('w');     std::string window = c?c:"";
  c = opt->getValue('v');     std::string nwins = c?c:"";
	if(!IntervalSetup(skip,interval,window,nwins))
    {
    opt->printUsage();
    delete opt;
		return(0);
    }

  //Open the input file.
  FILE *fpi = stdin;
  fpi = fopen(opt->getArgv(0),"rb");
  if(!fpi)
    ExitError(1,"Unable to open input file %s\n",opt->getArgv(0));
  if(setvbuf (fpi, 0, _IOFBF, 1024*1024))
    ExitError(1,"Unable to set input stream buffer size\n");
  ArchFilePath = opt->getArgv(0);
  
  //Unless inhibited by the --nointerp option, open a second pointer to the 
  //input file that will be used to look ahead for TCP packets for the purpose 
  //of interpolating the free running clock in a way that compensates for 
  //clock drift using the RTC clock as truth.
  bool InterpTCP = !opt->getFlag("nointerp");
  FILE *fp2 = NULL;
  if(InterpTCP)
    {
    //fp2 = fdopen (dup (fileno (fpi)), "rb");
    fp2 = fopen(opt->getArgv(0),"rb");
    if(!fp2)
      ExitError(1,"Unable to open read-ahead on input file %s\n",opt->getArgv(0));
    if(setvbuf (fp2, 0, _IOFBF, 1024*1024))
      ExitError(1,"Unable to set input read-ahead stream buffer size\n");
    }
  
  //Find out if we need to write headers into the TCP and Data files.
  bool WriteHdrs = opt->getFlag('h');

  //Find out if we need to include file offset in packet files
  bool InclOffset = opt->getFlag('O');

  //Find out if we need to suppress milliseconds following the timestamp
  //string in timestamped-line mode.
  SuppressMSec = opt->getFlag('S');

  //Find out if dat files (-d) should have only a single byte per line.
  bool DatBytePerLine = opt->getFlag("dat-bpl");
  
  //Default format for timestamped line output timestamps is overridden
  //if provided.
  TFormat = "%Y %m %d %H %M %S ";
  if(opt->getValue('N'))
    TFormat = opt->getValue('N');

  //The SSR doesn't use timezones, so assume all time calculations are
  //based on UTC.
  putenv ("TZ=UTC+0");
  tzset ();

  //Open the possible output files based on user specification.
  char *fname;
  FILE *fpr=NULL;        //raw output
  FILE *fpt=NULL;        //time output
  FILE *fpd=NULL;        //data output
  FILE *fpm=NULL;        //mixed output
  FILE *fpn=NULL;        //timestamped line output
  lua_State *L=NULL;    //external lua parser

  //raw output file
  fname = opt->getValue('r');
  if(fname) 
    {
    fpr = fopen(fname,"wb"); 
    if(!fpr) ExitError(1,"Unable to open raw data output file %s\n",fname);
    }
  
  //lua parser script  
  fname = opt->getValue('x');
  if(fname)
    L = LuaSetup(fname);
     
  //Time correlation output file
  fname = opt->getValue('t');
  if(fname) 
    {
    fpt = fopen(fname,"wt"); 
    if(!fpt) ExitError(1,"Unable to open time correlation output file %s\n",fname);
    if(WriteHdrs)
      fprintf(fpt,"RunTime(ms)%sYear Month Day Hour Minute Second\n",InclOffset?" Offset ":" ");
    }
    
  //Tagged data output file
  fname = opt->getValue('d');
  if(fname) 
    {
    fpd = fopen(fname,"wt"); 
    if(!fpd) ExitError(1,"Unable to open tagged data output file %s\n",fname);
    if(WriteHdrs)
      if(DatBytePerLine)
        fprintf(fpd,"RunTime(ms)%sHexByte\n",InclOffset?" Offset ":" ");
      else
        fprintf(fpd,"RunTime(ms)%scount HexBytes\n",InclOffset?" Offset ":" ");
    }
  
  //Mixed output file (tagged data and TCP packets)
  fname = opt->getValue('m');
  if(fname) 
    {
    fpm = fopen(fname,"wt"); 
    if(!fpm) ExitError(1,"Unable to open tagged mixed output file %s\n",fname);
    }
    
  //Tagged line output
  fname = opt->getValue('n');
  if(fname)
    {
    fpn = fopen(fname,"wb");
    if(!fpn) ExitError(1,"Unable to open tagged line output file %s\n",fname);
    }

  //------------------------ data processing ----------------------------    
  std::string pkt;
  int ch;
  int state = 0;
  unsigned char inbuf[100];       //MAX BYTES PER FRAME
  unsigned char *p;
  unsigned long rt_sec;
  unsigned long offset;
  tTCP PrevTCP = {0,2000,1,1,0,0,0,0}; //Latest Time Correlation Pkt encountered
  tTCP NextTCP = {0};

  //Using the look-ahead pointer, fetch the next TCP.  There must be at least
  //one, so if a null year is returned, that is an error.
  if(InterpTCP)
    {
    NextTCP = GetNextTCP(fp2);
    if(NextTCP.RT == 0)
      ExitError(1,"Error: A TCP was not found in %s\n",opt->getArgv(0));
    }
    
  for(;;)
    {
    //Get the next packet from the archive file.
    pkt = GetPacket(fpi,&offset);
    if(pkt.empty())
      break;
    
    //Good packet.  Parse it, writing output as appropriate.
    if(((unsigned char)pkt[1]) == 0xA3)
      {
      //Set this as the previous TCP and fetch the next one with 
      //the look-ahead pointer.
      PrevTCP = ParseTCP(pkt);
      if(InterpTCP)
        NextTCP = GetNextTCP(fp2);

      //Only bother creating output if we have a file to write to.
      if(fpt || fpm)
        {
        //Write to files
        char buf[200];
        if(InclOffset)
          sprintf(buf,"%lu %lu %hu %hu %hu %hu %hu %hu.%03hu",
                  PrevTCP.RT, offset,
                  PrevTCP.year, PrevTCP.month, PrevTCP.day,
                  PrevTCP.hour, PrevTCP.min, PrevTCP.sec, PrevTCP.msec);
        else
          sprintf(buf,"%lu %hu %hu %hu %hu %hu %hu.%03hu",
                  PrevTCP.RT,
                  PrevTCP.year, PrevTCP.month, PrevTCP.day,
                  PrevTCP.hour, PrevTCP.min, PrevTCP.sec, PrevTCP.msec);
        if(fpt)
          fprintf(fpt,"%s\n",buf);
        if(fpm)
          fprintf(fpm,"A3 %s\n",buf);
        }
      }
    else if(((unsigned char)pkt[1]) == 0xA2)
      {
      //Time Tagged Data Packet.  Parse as follows:
      //  Data Packet
      //    0x82A2    2       Packet start sequence (different from jBin)
      //    rt_sec    4       Current run-time in seconds (RunTime%1000).
      //    --- repeat block for the current whole second ---
      //    ms/count  2       upper 9 bits is milliseconds/2 (max_ms = 999ms)
      //                      lower 7 bits is count (max=127)
      //    bytes     n       upto n bytes.  Could get 92.160 bytes/frame at 460kbaud.
      //    --- end repeat ---
      //    0xFFFF    2       End sequence (something disambiguous with ms/count)
      //    cksum     2       Fletcher checksum, starting with rt_sec through end seq.
      //Only bother parsing if we have a file to write to.
      if(fpr || fpd || fpm || fpn || L)
        {
        unsigned long RT_sec;
        unsigned short uh;
        int index;

        //Extract Run Time (sec)
        RT_sec =   (((unsigned char)pkt[2])<<24)
                 | (((unsigned char)pkt[3])<<16)
                 | (((unsigned char)pkt[4])<<8)
                 | (((unsigned char)pkt[5]));

        //Get the ms/count field.
        index = 6;
        uh =   (((unsigned char)pkt[index])<<8)
             | (((unsigned char)pkt[index+1]));
        index += 2;

        //Parse the sub blocks until we reach the end
        while(uh != 0xFFFF)
          {
          //Break the sub-packet header into msec/count
          unsigned short msec, count;
          msec = (uh>>7)*2;
          count = uh & 0x7F;

          //Get the characters
          std:string subpkt = pkt.substr(index,count);
          index += count;

          //Since the high speed unit (bauds > 635k) can write more than
          //one block per frame, we need to peek into the next block to see
          //if it has same msec stamp.  If it does, consider it part of this
          //frame.
          uh =   (((unsigned char)pkt[index])<<8)
               | (((unsigned char)pkt[index+1]));
          if((uh>>7)*2 == msec)
            {
            //This sub block is part of the same frame.  Add it to our data.
            index += 2;
            
            unsigned short ncount = uh&0x7F;
            subpkt += pkt.substr(index,ncount);
            index += ncount;
            count += ncount;
            }
          
          //Write raw data
          if(fpr)
            fwrite(subpkt.c_str(), 1, count, fpr);

          //Parse with external parser
          if(L)
            {
            //Get a timestamp string for this subpacket to provide to the
            //external parser along with the subpacket data.  The parser
            //will receive a double with RT(sec.msec), a string with the
            //formatted timestamp, and the subpacket data.
            tTCP tcp = GetSubPacketTime(PrevTCP, NextTCP, RT_sec, msec);
            std::string ts = GetLineTimeStamp(tcp, TFormat, SuppressMSec);
            
            LuaParse(L,subpkt,RT_sec+msec/1000.0L,ts);
            }
          
          //Now handle the time tagged output
          if(fpd || fpm)
            {
            //Create a hex equivalend of the data in subpkt.
            std::string hexpkt;
            std::string::iterator it;
            for(it = subpkt.begin(); it < subpkt.end(); ++it)
              {
              unsigned char uc, uch, ucl;
              uc = (unsigned char)(*it);
              uch = uc>>4;      //high nibble
              ucl = uc&0xF;     //low nibble
              uch = (uch < 10)?(uch+48):(uch+65-10);  //To Hex
              ucl = (ucl < 10)?(ucl+48):(ucl+65-10);  //To Hex
              hexpkt.push_back(uch);
              hexpkt.push_back(ucl);
              }

            //Now write the output line
            if(InclOffset)
              {
              if(DatBytePerLine)
                for(int i=0;i<count;++i)
                  WriteDMLine(fpd,fpm,"%lu%03hu %lu %s",RT_sec, msec, offset, hexpkt.substr(2*i,2).c_str());
              else
                WriteDMLine(fpd,fpm,"%lu%03hu %lu %hu %s",RT_sec, msec, offset, count, hexpkt.c_str());
              }
            else
              {
              if(DatBytePerLine)
                for(int i=0;i<count;++i)
                  WriteDMLine(fpd,fpm,"%lu%03hu %s",RT_sec, msec, hexpkt.substr(2*i,2).c_str());
              else
                WriteDMLine(fpd,fpm,"%lu%03hu %hu %s",RT_sec, msec, count, hexpkt.c_str());
              }

            }
          else if(fpn)       //Handle timestamped line output
            {
            //Firstly, if we have data packets without having gotten a
            //TCP, then we have a problem.  Discard those instead of trying
            //to guess at a timestamp.  Only preform the ops below if PrevTCP
            //is valid.
            if(PrevTCP.RT)
              {
              //The subpackets should contain text lines that need to be stamped.
              //Assume, for the moment, that we need only to stamp lines that
              //have content.  If we encounter an newline, we will set a flag
              //to Stamp-On-Next-Content and then produce/insert the time stamp
              //when a non newline/CR character is encountered.
              std::string::iterator it;
              static bool StampOnNextContent = true;
              static bool OutputEnabled = true;           //can be made false by intervals
              static unsigned long TSLinesGenerated=0;
              for(it = subpkt.begin(); it < subpkt.end(); ++it)
                {
                unsigned char uc = (unsigned char)(*it);
  
                //Whenever a newline/CR is received, arm StampOnNextContent
                if(uc==0xA || uc==0xD)
                  StampOnNextContent=true;
                else if(StampOnNextContent)
                  {
                  //StampOnNextContent is armed, and a non NL/CR byte has been
                  //received.  Insert the timestamp in the output file.
  
                  //Get the equivalent time of the current subpacket.
                  //Sending a zero TCP repliates the pre interpolation 
                  //(sttp v1.6) behavior.
                  //optionally send a null TCP packst for NextTCP.
                  tTCP tcp = GetSubPacketTime(PrevTCP, NextTCP, RT_sec, msec);
  
                  //Create text for the timestamp using TFormat
                  std::string ts = GetLineTimeStamp(tcp, TFormat, SuppressMSec);
                  ts.push_back(' ');
                  ++TSLinesGenerated;
  
                  //Process intervals for the possibility of disabling output.
                  OutputEnabled = IntervalWriteEnabled(tcp,TSLinesGenerated);
  
                  //Write the timestamp to file, and disarm StampOnNextContent
                  if(OutputEnabled)
                    fputs(ts.c_str(),fpn);
                  StampOnNextContent = false;
                  }
  
                //Put all subpacket bytes into the output file.
                if(OutputEnabled)
                  fputc((int)uc,fpn);
                }
              }
            }


          //Read the next ms/count or 0xFFFF word.
          uh =   (((unsigned char)pkt[index])<<8)
               | (((unsigned char)pkt[index+1]));
          index += 2;
          }
        }
      }
    }

  //Clean up
  delete opt;
  if(fpi) fclose(fpi);
  if(fp2) fclose(fp2);
  if(fpr) fclose(fpr);
  if(fpt) fclose(fpt);
  if(fpd) fclose(fpd);
  if(fpm) fclose(fpm);
  if(fpn) fclose(fpn);

  if(L) lua_close(L);

  return(0);
  }

//========================================================================
//                             GetPacket
//========================================================================
//Gets a packet from fpi and returns it in a string.  'loc' is filled with
//the start location of the packet (ftell).
std::string GetPacket(FILE *fpi, unsigned long *loc)
  {
  unsigned char uc;
  unsigned short uh;
  int ch;
  int count;
  int state=0;
  std::string os;                       //empty output string.
  
  while((ch=fgetc(fpi)) != EOF)
    {
    switch(state)
      {
      case 0:             //waiting for start char 0x82
        if(ch != 0x82)
          continue;
        *loc = ftell(fpi);
        os.push_back((char)ch);
        ++state;
        break;
      case 1:             //waiting for next char 0xA2 or 0xA3
        if((ch != 0xA2) && (ch != 0xA3))
          {
          os.clear();
          state = 0;
          continue;
          }
        //Split based on which packet we're receiving
        os.push_back((char)ch);
        if(ch == 0xA2)
          state = 100;
        else if(ch == 0xA3)
          {
          count = 12;       //need to rx 12, see state 200.
          state = 200;
          }
        break;
      //------------------------------------------------
      //State machine for a data packet
      //Data Packet
      //  0x82A2    2       Packet start sequence (different from jBin)
      //  rt_sec    4       Current run-time in seconds (RunTime%1000).
      //  --- repeat block for the current whole second ---
      //  ms/count  2       upper 9 bits is milliseconds/2 (max_ms = 999ms)
      //                    lower 7 bits is count (max=127)
      //  bytes     n       upto n bytes.  Could get 92.160 bytes/frame at 460kbaud.
      //  --- end repeat ---
      //  0xFFFF    2       End sequence (something disambiguous with ms/count)
      //  cksum     2       Fletcher checksum, starting with rt_sec through end seq.
      case 100:     //Receive rt_sec
      case 101:
      case 102:
      case 103:
        os.push_back((char)ch);
        ++state;
        break;
      case 104:     //Get 2 bytes, either ms/count or 0xFFFF
        os.push_back((char)ch);
        ++state;
        break;
      case 105:     //Get 2nd byte and do some validation
        os.push_back((char)ch);
        uh = (unsigned char)(os[os.size()-2]);
        uh = (uh<<8) | (unsigned char)(os[os.size()-1]);
        if(uh == 0xFFFF)
          state = 110;
        else
          {
          //This should be ms/count.  Make sure ms is not > 999.
          int msec = (uh>>7)*2;
          if(msec > 999)
            {
            state = 0;
            fseek(fpi, -(os.size()-1), SEEK_CUR);
            os.clear();
            }
          else
            {
            count = uh&0x7F;
            if(count <= 0)
              {
              state = 0;
              fseek(fpi, -(os.size()-1), SEEK_CUR);
              os.clear();
              }
            else
              ++state;
            } 
          }
        break;
      case 106:         //getting 'count' bytes
        os.push_back((char)ch);
        if(--count == 0)
          state = 104;
        break;
      case 110:         //Receive the checksum
        os.push_back((char)ch);
        ++state;
        break;
      case 111:         //Receive the last cksum byte
        os.push_back((char)ch);
        if(ValidateCksum(os))
          {
          //fprintf(stderr,"[%ld]%ld\n",state,ftell(fpi));
          return(os);
          }
        else
          {
          fseek(fpi, -(os.size()-1), SEEK_CUR);
          os.clear();
          state = 0;
          }
        break;

      //------------------------------------------------
      //State machine for a time correlation packet
      //Time Correlation Packet
      //  0x82A3    2     Packet start sequence (differentiate from data packet)
      //  RunTime   4     Current run-time in milliseconds (RunTime)
      //  RTCTime   6     Encode t_RTC date and time fields. year to msec.
      //  cksum     2     Fletcher checksum, starting at RunTime thru end of RTCTime     
      case 200:     //Receive rt_sec
        os.push_back((char)ch);
        if(--count == 0)
          {
          if(ValidateCksum(os))
            {
            //fprintf(stderr,"[%ld]%ld\n",state,ftell(fpi));
            return(os);
            }
          else
            {
            fseek(fpi, -(os.size()-1), SEEK_CUR);
            os.clear();
            state = 0;
            }
          }
        break;
      }
    }

  //If we get here, we likely reached the end of file, perhaps in the middle
  //of a packet.  Clear the output string and return the empty packet.
  os.clear();
  //fprintf(stderr,"[%ld]%ld\n",state,ftell(fpi));
  return(os);
  }

//========================================================================
//                             ValidateCksum
//========================================================================
bool ValidateCksum(std::string s)
  {
  //This assumes s holds a packet, with the first two bytes as the packet
  //header (not included in the checksum) and the last two bytes are the
  //packet checksum.  So, we need to calculate a checksum on everything
  //in between.
  unsigned char ck0 = 0;
  unsigned char ck1 = 0;
  int bytes = s.length() - 4;
  int i;
  for(i=2; i < bytes+2; ++i)
    {
    ck0 += (unsigned char)s[i];
    ck1 += ck0;
    }
  return(   (ck0 == (unsigned char)s[s.length()-2])
         && (ck1 == (unsigned char)s[s.length()-1])); 
  }

//========================================================================
//                          ExitError
//========================================================================
void ExitError(int retval, const char *fmt, ...)
  {
  va_list argp;
  va_start(argp,fmt);
  vfprintf(stderr, fmt, argp);
  exit(retval);
  }

//========================================================================
//                        BaseFileName
//========================================================================
std::string BaseFileName(const char *path)
  {
  char name[_MAX_FNAME], ext[_MAX_EXT];
  _splitpath(path,0,0,name,ext);
	return(std::string(name) + std::string(ext));
  }

//========================================================================
//                        ParseTCP
//========================================================================
//Given a string of bytes returned from a TCP fetched with GetPacket, parse
//the data into a TCP and return it.
tTCP ParseTCP(std::string pkt)
  {
  tTCP TCP;
  
  //Time Correlation Packet.  Parse as follows:
  //  Time Correlation Packet
  //    0x82A3    2     Packet start sequence (differentiate from data packet)
  //    RunTime   4     Current run-time in milliseconds (RunTime)
  //    RTCTime   6     Encode t_RTC date and time fields. year to msec.
  //    cksum     2     Fletcher checksum, starting at RunTime thru end of RTCTime
  //
  //  For Time Correlation Packets, the real-time will be encoded as follows:
  //    ushort   15:4     12 bits   year  1-4095
  //              3:0     4 bits    month 1-12
  //    ushort  15:11     5 bits    day   1-31
  //             10:6     5 bits    hour  0-23
  //              5:0     6 bits    min   0-59
  //    ushort  15:10     6 bits    sec   0-59
  //              9:0     10 bits   msec  0-999
  unsigned short uh;

  //Extract run time (msec)
  TCP.RT =   (((unsigned char)pkt[2])<<24)
           | (((unsigned char)pkt[3])<<16)
           | (((unsigned char)pkt[4])<<8)
           | (((unsigned char)pkt[5]));

  //Extract year, month
  uh =   (((unsigned char)pkt[6])<<8)
       | (((unsigned char)pkt[7]));
  TCP.year = uh>>4;
  TCP.month = uh&0xF;

  //Extract day, hour, min
  uh =   (((unsigned char)pkt[8])<<8)
       | (((unsigned char)pkt[9]));
  TCP.day = uh>>11;
  TCP.hour = (uh>>6)&0x1F;
  TCP.min = uh&0x3F;

  //Extract sec, msec
  uh =   (((unsigned char)pkt[10])<<8)
       | (((unsigned char)pkt[11]));
  TCP.sec = uh>>10;
  TCP.msec = uh&0x3FF;

  return(TCP);
  } 

//========================================================================
//                        GetNextTCP
//========================================================================
//Supports look-ahead for A3 TCP packets.  To prevent reinventing the wheel
//we will simply use the exiting GetPacket function and discard any packet
//that isn't a TCP
tTCP GetNextTCP(FILE *fp)
  {
  std::string pkt;
  unsigned long offset;
  tTCP TCP={0};
  
  //Get packets from the file until we get a TCP, or we hit the end of file.
  for(;;)
    {
    pkt = GetPacket(fp,&offset);
    if(pkt.empty())
      break;
    
    //Good packet.  Parse it, writing output as appropriate.
    if(((unsigned char)pkt[1]) == 0xA3)
      {
      TCP=ParseTCP(pkt);
      break;
      }
    }
  return(TCP);
  }

//========================================================================
//                        GetSubPacketTime
//========================================================================
tTCP firstTCP={0};           //Log the earliest time in the file.

//Given a timestamp (RT_sec, msec) from a Time Tagged Data Packet (A2),
//estimate the clock time based on the most recent Time Correlation
//Packet (PrevTCP) and the next one (NextTCP).  Return struct tm with the result.
//With v1.6, this function supports interpolation of timestamp using Prev and 
//Next TCPs to account for drift of the timestamp clock.  It will handle 
//two cases:
//  Prev=0/Next=TCP (file start without TCP), <= Invalid case, ignoring.
//  Prev=TCP/Next=TCP,
//  Prev=TCP/Next=0 (file end or interpolation disabled)
//  Also in the middle case if next TCP timestamp < Prev TCP timestamp, then
//  perhaps power was cycled and file append was enabled.  Treat like Next=0
//  same as end of file and interpolate only from Prev using slope of 1.

tTCP GetSubPacketTime(tTCP &PrevTCP, tTCP &NextTCP, unsigned long RT_sec, unsigned short msec)
  {
  //Get the time delta between the last TCP and the current time stamp.
  //Note that RT in tTCP is in msec.
  //unsigned long dt = (RT_sec*1000 + msec) - PrevTCP.RT;

  //Get a scaled dt (free running clock time since previous TCP) to compensate 
  //for drift using the more accurate RTC stamps in PrevTCP and NextTCP
  unsigned long dt = ScaledDT((RT_sec*1000 + msec), PrevTCP, NextTCP);

  //Get the time delta seconds and msec
  unsigned long dt_sec = dt/1000;
  unsigned long dt_msec = dt%1000;

  //Create a tm struct from PrevTCP.
  struct tm T;
  T.tm_year = PrevTCP.year - 1900;  //tm_year int years since 1900
  T.tm_mon  = PrevTCP.month - 1;    //tm_mon int months since January 0-11
  T.tm_mday = PrevTCP.day;          //tm_mday int day of the month 1-31
  T.tm_hour = PrevTCP.hour;         //tm_hour int hours since midnight 0-23
  T.tm_min  = PrevTCP.min;          //tm_min  int minutes after the hour 0-59
  T.tm_sec  = PrevTCP.sec;          //tm_sec  int seconds after the minute	0-60*
  //TCP has msec that struct tm will not allow for.  That means PrevTCP is
  //some number of milliseconds nearer in the past than we can represent.
  //Account for that difference.
  dt_msec += PrevTCP.msec;
  if(dt_msec > 999)
    {
    dt_msec -= 1000;
    dt_sec += 1;
    }

  //Get time_t corresponding to PrevTCP (whole second)
  T.tm_isdst=0;                    //prevent use of daylight savings time
  time_t tt = mktime(&T);

  //Increment to the current subpacket time (whole second).
  tt += dt_sec;

  //Create T at the subpacket time
  T = *gmtime(&tt);

  //Create and return a tTCP struct with the new time
  tTCP tcp;
  tcp.year = T.tm_year + 1900;
  tcp.month = T.tm_mon + 1;
  tcp.day = T.tm_mday;
  tcp.hour = T.tm_hour;
  tcp.min = T.tm_min;
  tcp.sec = T.tm_sec;
  tcp.msec = dt_msec;

  //If this is the first TCP, record it.
  if(firstTCP.RT == 0)
    firstTCP = tcp;

  return(tcp);
  }


//========================================================================
//                        ScaledDT
//========================================================================
//Convert dt into a corrected dt based on PrevTCP and NextTCP, assuming
//that dt lies between them.  If dt does not, return it unchanged.  If it
//is between, we're basically doing
//       dt = ((dt - Prev.RT)/(Next.RT - Prev.RT))*(Next.RTC - Prev.RTC)
unsigned long ScaledDT(unsigned long RT, tTCP &PrevTCP, tTCP &NextTCP)
  {
  unsigned long dt;
  
  //As a default value, calculate dt the old way (pre Interp)
  dt = RT - PrevTCP.RT;
  
  //If dt is between our two TCPs, interpolate it.
  if((RT >= PrevTCP.RT) && (RT <= NextTCP.RT))
    {
    //Start by calculating ((dt - Prev.RT)/(Next.RT - Prev.RT)) which is
    //the fraction of RT time dt is between the TCPs.
    double a,b, pct;
    a = RT - PrevTCP.RT;
    b = NextTCP.RT - PrevTCP.RT;
    if(b==0)
      return(dt);
    pct = a/b;
    
    //Calcualte the number of RTC seconds between the TCPs
    unsigned long dRTC = GetTCPDiff_msec(NextTCP,PrevTCP);
    
    //Calculate the scaled dt
    dt = pct*dRTC;
    }
  return(dt);
  }

//========================================================================
//                        GetTCPDiff_msec
//========================================================================
unsigned long GetTCPDiff_msec(tTCP &newer, tTCP &older)
  {
  //Create a tm struct and time_t from newer.
  struct tm Tn;
  Tn.tm_year = newer.year - 1900;  //tm_year int years since 1900
  Tn.tm_mon  = newer.month - 1;    //tm_mon int months since January 0-11
  Tn.tm_mday = newer.day;          //tm_mday int day of the month 1-31
  Tn.tm_hour = newer.hour;         //tm_hour int hours since midnight 0-23
  Tn.tm_min  = newer.min;          //tm_min  int minutes after the hour 0-59
  Tn.tm_sec  = newer.sec;          //tm_sec  int seconds after the minute	0-60*
  Tn.tm_isdst=0;                    //prevent use of daylight savings time
  time_t tn = mktime(&Tn);

  //Create a tm struct and time_t from older.
  struct tm To;
  To.tm_year = older.year - 1900;  //tm_year int years since 1900
  To.tm_mon  = older.month - 1;    //tm_mon int months since January 0-11
  To.tm_mday = older.day;          //tm_mday int day of the month 1-31
  To.tm_hour = older.hour;         //tm_hour int hours since midnight 0-23
  To.tm_min  = older.min;          //tm_min  int minutes after the hour 0-59
  To.tm_sec  = older.sec;          //tm_sec  int seconds after the minute	0-60*
  To.tm_isdst=0;                    //prevent use of daylight savings time
  time_t to = mktime(&To);

  unsigned long msec_diff = (tn-to)*1000;
  
  //Deal with incremental msec
  msec_diff += (int)newer.msec - (int)older.msec;
  return(msec_diff);
  }
  
//========================================================================
//                        GetLineTimeStamp
//========================================================================
//Given a time as specified in a tTCP (time correlation packet) struct,
//create a text representation based on the specified format.
std::string GetLineTimeStamp(tTCP &tcp, std::string &format, bool SuppressMSec)
  {
  int n;
  struct tm T;
  char tbuf[128];

  T.tm_year = tcp.year - 1900;  //tm_year int years since 1900
  T.tm_mon  = tcp.month - 1;    //tm_mon int months since January 0-11
  T.tm_mday = tcp.day;          //tm_mday int day of the month 1-31
  T.tm_hour = tcp.hour;         //tm_hour int hours since midnight 0-23
  T.tm_min  = tcp.min;          //tm_min  int minutes after the hour 0-59
  T.tm_sec  = tcp.sec;          //tm_sec  int seconds after the minute	0-60*

  //Generate a text time stamp in tbuf.  Abort if error.
  n=strftime(tbuf, sizeof(tbuf), format.c_str(), &T);
  if(!n)
    ExitError(1,"Error creating timestamp for text line.\n");

  //Copy to the return timestamp string.
  std::string ts(tbuf);

  //Append msec to the string if not suppressed on command line.
  if(!SuppressMSec)
    {
    sprintf(tbuf,"%03hu",tcp.msec);
    ts += tbuf;
    }

  //Return the time stamp string.
  return(ts);
  }


//If Skip, Interval, or Window is negative, then the mode is to skip lines 
//instead of seconds.  NWins is always an integer - number of windows.
int Skip;
int Interval;
int Window;
int NWins;

//========================================================================
//                        IntervalSetup
//========================================================================
//Return false if we run into a problem, true if good to go.
bool IntervalSetup(std::string skip, std::string interval, std::string window, std::string nwins)
  {
  //Each of these arguments should be an integer optionally suffixed with 'L'.
  //With no suffix, the integer denotes seconds. With 'L', it denotes lines.
  Skip = atoi(skip.c_str());
  if(skip.length()>0 and skip[skip.length()-1]=='L')
    Skip = -Skip;

  Interval = atoi(interval.c_str());
  if(interval.length()>0 and interval[interval.length()-1]=='L')
    Interval = -Interval;
    
  Window = atoi(window.c_str());
  if(window.length()>0 and window[window.length()-1]=='L')
    Window = -Window;

  NWins = atoi(nwins.c_str());

  return(true);
  }

//========================================================================
//                        IntervalWriteEnabled
//========================================================================
//Returns true if we should perform the write represented by the arguments.
bool IntervalWriteEnabled(tTCP &tcp, unsigned long TSLinesGenerated)
  {
  //If we are less than the appropriate distance into the file (skip) then
  //return false.
  if(Skip)
    {
    //If positive, check for seconds elapsed from start of file.
    if(Skip>0)
      {
      if(GetTCPDiff_msec(tcp,firstTCP)<(Skip*1000))
        return(false);
      }
    else
      {
      //Check for lines generated since start of file.
      if(TSLinesGenerated < -Skip)
        return(false);
      }
    //If we get this far, then we have just gotten past the skip period.
    //Reset firstTCP and TSLinesGenerated to start the interval assessment
    //from here, and disable Skip going forward.
    firstTCP = tcp;
    TSLinesGenerated = 0;
    Skip = 0;  
    }

  //Next, if we are not inside a window at a valid interval, return false.
  //To begin with, if interval or window is zero, then return true.
  if(Interval==0 or Window==0)
    return(true);
    
  //Determine the base of the current interval - time or lines
  unsigned long CurrentTime = GetTCPDiff_msec(tcp,firstTCP); 
  static unsigned long CurrentIntervalNumber = 0;
  static unsigned long CurrentIntervalStartTime = CurrentTime;
  static unsigned long CurrentIntervalStartLines = TSLinesGenerated;
  if(Interval > 0)
    {
    //The interval is time (seconds).
    unsigned long k = CurrentTime/(Interval*1000);
    //If this is in a new interval, reset our base.
    if(CurrentIntervalNumber != k)
      {
      CurrentIntervalNumber = k;
      CurrentIntervalStartTime = CurrentIntervalNumber*Interval*1000;
      CurrentIntervalStartLines = TSLinesGenerated;
      }
    }
  else
    {
    //The interval is lines
    unsigned long k = TSLinesGenerated/(-Interval);
    //If this is in a new interval, reset our base.
    if(CurrentIntervalNumber != k)
      {
      CurrentIntervalNumber = k;
      CurrentIntervalStartTime = CurrentTime;  //time of this packet
      CurrentIntervalStartLines = TSLinesGenerated;
      }
    }

  //If the current interval number is greater than the number of windows
  //we want to capture, we're done writing output.
  if(NWins && (CurrentIntervalNumber >= NWins))
    return(false);

  //Now, determine whether we are in the window.
  if(Window>0)
    {
    //Time window.  See if we are within Window seconds of the base.
    if((CurrentTime - CurrentIntervalStartTime) > Window*1000)
      return(false);
    }
  else
    {
    //Lines window.  See if we've gone past the number of window lines
    if((TSLinesGenerated-CurrentIntervalStartLines) >= -Window)
      return(false);  
    } 


  return(true);
  }
    
//========================================================================
//                          WriteDMLine
//========================================================================
//Convenience function to encapsuate repeated logic.  This function
//takes the fpd and fpm (data and mixed) file pointers along with the
//information to be written (in printf formate) and takes care of doing the
//write operation
void WriteDMLine(FILE *fpd, FILE *fpm, const char *fmt, ...)
  {
  char buf[600];
  va_list argp;
  va_start(argp,fmt);
  vsprintf(buf,fmt,argp);
  if(fpd)
    fprintf(fpd,"%s\n",buf);
  if(fpm)
    fprintf(fpm,"A2 %s\n",buf);
  }

//========================================================================
//                    Lua Environment Functions
//========================================================================
//A few functions that we'd like to make available to the Lua scripts.
//  sttp_setTSFormat(<strftime format string>)
//  sttp_getPaths()  returns a table with relevant paths in fields:
//          ArchiveFile, ArchivePath, ArchiveName, ArchiveExt, cwd

extern "C" {

//Sets TFormat from the lua script to control the timestamp format without
//having to provide the -N argument to the sttp command line. 
static int sttp_setTSFormat(lua_State *L)  //[-2,+0]
  {
  TFormat = luaL_checkstring(L,-2);         //get string provided on stack
  SuppressMSec = lua_toboolean(L,-1);   //bool, SuppressMSec
  return(0);                            //no results pushed to stack
  }


static int sttp_getPaths(lua_State *L)  //[-0,+1]
  {
  char drive[_MAX_DRIVE], dir[_MAX_DIR], name[_MAX_FNAME], ext[_MAX_EXT];
  char absPath[_MAX_PATH];
  _fullpath(absPath,ArchFilePath.c_str(),_MAX_PATH);
  _splitpath(absPath,drive,dir,name,ext);
  
  lua_newtable(L);
  lua_pushstring(L,absPath);
  lua_setfield(L,-2,"ArchiveFile");
  lua_pushstring(L,(std::string(drive)+std::string(dir)).c_str());
  lua_setfield(L,-2,"ArchivePath");
  lua_pushstring(L,name);
  lua_setfield(L,-2,"ArchiveName");
  lua_pushstring(L,ext);
  lua_setfield(L,-2,"ArchiveExt");
  
  _getcwd(absPath,_MAX_PATH);
  lua_pushstring(L,absPath);
  lua_setfield(L,-2,"cwd");
  
  return(1);                          //one result pushed to stack
  }

} //End extern "C"
  
//========================================================================
//                          LuaSetup
//========================================================================
//LuaSetup encapsulates the code needed to setup the Lua parser, install
//the custom sttp functions for scripts to use, and load the user script.
lua_State *LuaSetup(char *fname)
  {
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  
  //Install custom functions
  lua_pushcfunction(L, sttp_setTSFormat);
  lua_setglobal(L,"sttp_setTSFormat");
  lua_pushcfunction(L, sttp_getPaths);
  lua_setglobal(L,"sttp_getPaths");
  
  if(luaL_dofile(L,fname))
    {
    fprintf(stderr,"Error loading external parser file %s\n",fname);
    ExitError(2,lua_tostring(L,-1));
    }
  return(L);
  }


//========================================================================
//                          LuaParse
//========================================================================
void LuaParse(lua_State *L, std::string &subpkt, double RT, std::string &TS)
  {
  //Get the function onto the stack
  lua_getglobal(L,"ParseData");
  if(!lua_isfunction(L,-1))
    ExitError(2,"ParseData function not found in external parser.");

  //Push the parameters onto the stack
  lua_pushnumber(L,RT);                               //RT as double
  lua_pushstring(L,TS.c_str());                       //Timestamp str
  lua_pushlstring(L,subpkt.c_str(),subpkt.length());  //Data
  
  //Call the parser function in the Lua module
  if(lua_pcall(L,3,0,0))
    ExitError(2,lua_tostring(L,-1));
  lua_settop(L,0);
  }
