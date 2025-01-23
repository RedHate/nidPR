//Mips.h

#include "headers/cdefs.h"

#define S 0
#define T 1
#define D 2

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pspdebug.h>

unsigned char mipsNum[16];																																																  //fp
unsigned char *mipsRegisterArray[]={"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "s8", "ra"};
//unsigned char *mipsRegisterArray[]={"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"};
unsigned char *specialRegisterArray[]={ "ixltg", "ixldt", "bxlbt", "$3", "ixstg", "ixsdt", "bxsbt", "ixin", "bpr", "$9", "bhinbt", "ihin", "bfh", "$13", "ifl", "$15", "dxltg", "dxldt", "dxstg", "dxsdt", "dxwbin", "$21", "dxin", "$23", "dhwbinv", "$25", "dhin", "$27", "dhwoin", "$29", "$30", "$31" };
unsigned char *floatRegisterArray[]={"$f0","$f1","$f2","$f3","$f4","$f5","$f6","$f7","$f8","$f9","$f10","$f11","$f12","$f13","$f14","$f15","$f16","$f17","$f18","$f19","$f20","$f21","$f22","$f23","$f24","$f25","$f26","$f27","$f28","$f29","$f30","$f31",};
unsigned char *labelRegisterArray[]={"Index","Random","EntryLo0","EntryLo1","Context","PageMask","Wired","$7","BadVAddr","Count","EntryHi","Compare","Compare","Cause","EPC","PRId","Config","LLAddr","WatchLo","WatchHi","XContent","$21","$22","BadPAddr","Debug","Perf","ECC","CacheERR","TagLo","TagHi","ErrorEPC","$31"};

void labelRegister(unsigned int a_opcode, unsigned char a_slot, unsigned char a_more)
{
  a_opcode=(a_opcode>>(6+(5*(3-a_slot)))) & 0x1F;
  pspDebugScreenSetTextColor(CYAN); pspDebugScreenPuts(labelRegisterArray[a_opcode]); pspDebugScreenSetTextColor(WHITE);
  
  if(a_more) pspDebugScreenPuts(", ");
}

void floatRegister(unsigned int a_opcode, unsigned char a_slot, unsigned char a_more)
{
  a_opcode=(a_opcode>>(6+(5*(3-a_slot)))) & 0x1F;
  pspDebugScreenSetTextColor(CYAN); pspDebugScreenPuts(floatRegisterArray[a_opcode]); pspDebugScreenSetTextColor(WHITE);
  if(a_more) pspDebugScreenPuts(", ");
}

void specialRegister(unsigned int a_opcode, unsigned char a_slot, unsigned char a_more)
{
  a_opcode=(a_opcode>>(6+(5*(3-a_slot)))) & 0x1F;
  
  char buffer[256];
  if(a_opcode != 0x03 || 0x09 || 0x0D || 0x0F || 0x15 || 0x17 || 0x19 || 0x1B || 0x1D || 0x1E || 0x1F){
  	pspDebugScreenPuts(specialRegisterArray[a_opcode]);
  }
  else{
  	pspDebugScreenSetTextColor(MAGENTA);
  	pspDebugScreenPuts("$");
  	pspDebugScreenSetTextColor(YELLOW);
  	sprintf(buffer, "%2d", a_opcode); pspDebugScreenPuts(buffer);
  	pspDebugScreenSetTextColor(WHITE);
  }
  
  if(a_more) pspDebugScreenPuts(", ");
}

void mipsRegister(unsigned int a_opcode, unsigned char a_slot, unsigned char a_more)
{
  a_opcode=(a_opcode>>(6+(5*(3-a_slot)))) & 0x1F;
  pspDebugScreenSetTextColor(CYAN); pspDebugScreenPuts(mipsRegisterArray[a_opcode]);  pspDebugScreenSetTextColor(WHITE);
  if(a_more) pspDebugScreenPuts(", ");
}

void mipsNibble(unsigned int a_opcode, unsigned char a_slot, unsigned char a_more)
{
  a_opcode=(a_opcode>>(6+(5*(3-a_slot)))) & 0x1F;
  pspDebugScreenSetTextColor(MAGENTA);
  pspDebugScreenPuts("$");
  pspDebugScreenSetTextColor(YELLOW);
  sprintf(mipsNum, "%X", a_opcode); pspDebugScreenPuts(mipsNum);
  pspDebugScreenSetTextColor(WHITE);
  if(a_more) pspDebugScreenPuts(", ");
}

void mipsImm(unsigned int a_opcode, unsigned char a_slot, unsigned char a_more)
{
  if(a_slot==1)
  {
    a_opcode&=0x3FFFFFF;
    pspDebugScreenSetTextColor(MAGENTA);
    pspDebugScreenPuts("$");
    pspDebugScreenSetTextColor(YELLOW);
    sprintf(mipsNum, "%08X", ((a_opcode<<2))); pspDebugScreenPuts(mipsNum);
    pspDebugScreenSetTextColor(WHITE);
  }
  else
  {
    a_opcode&=0xFFFF;
    pspDebugScreenSetTextColor(MAGENTA);
    pspDebugScreenPuts("$");
    pspDebugScreenSetTextColor(YELLOW);
    sprintf(mipsNum, "%04X", a_opcode); pspDebugScreenPuts(mipsNum);
    pspDebugScreenSetTextColor(WHITE);
  }
  
  if(a_more) pspDebugScreenPuts(", ");
}

void mipsDec(unsigned int a_opcode,  unsigned int type, unsigned char a_slot, unsigned char a_more)
{
  if(a_slot==1)
  {
    a_opcode&=0x3FFFFFF;
    pspDebugScreenSetTextColor(MAGENTA);
    pspDebugScreenPuts("$");
    pspDebugScreenSetTextColor(YELLOW);
    switch(type){
    	case 0:
    		sprintf(mipsNum, "%010lu", ((a_opcode<<2))); pspDebugScreenPuts(mipsNum); pspDebugScreenSetTextColor(WHITE);
    	break;
    	case 1:
    		sprintf(mipsNum, "%x", ((a_opcode<<2))); pspDebugScreenPuts(mipsNum); pspDebugScreenSetTextColor(WHITE);
    	break;
    	case 2:
    		sprintf(mipsNum, "%d", ((a_opcode<<2))); pspDebugScreenPuts(mipsNum); pspDebugScreenSetTextColor(WHITE);
    		
    	break;
	}
  }
  else
  {
    a_opcode&=0xFFFF;
    pspDebugScreenSetTextColor(MAGENTA);
    pspDebugScreenPuts("$");
    pspDebugScreenSetTextColor(YELLOW);
    
    switch(type){
    	case 0:
    		sprintf(mipsNum, "%05lu", a_opcode); pspDebugScreenPuts(mipsNum); pspDebugScreenSetTextColor(WHITE);
    	break;
    	case 1:
    		sprintf(mipsNum, "%x", ((a_opcode<<2))); pspDebugScreenPuts(mipsNum); pspDebugScreenSetTextColor(WHITE);
    	break;
    	case 2:
    		sprintf(mipsNum, "%d", ((a_opcode<<2))); pspDebugScreenPuts(mipsNum); pspDebugScreenSetTextColor(WHITE);
    	break;
	}
    
    
    
  }
  
  if(a_more) pspDebugScreenPuts(", ");
}

void mipsDecode(unsigned int a_opcode)
{
  pspDebugScreenSetTextColor(MAGENTA);
  //Handle opcode
  switch((a_opcode & 0xFC000000) >> 26)
  {
    case 0x00:
      switch(a_opcode & 0x3F)
      {
        case 0x00:
          if(a_opcode == 0)
          {
            pspDebugScreenPuts("nop      ");
 			//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
          }
          else
          {
            pspDebugScreenPuts("sll      ");
            mipsRegister(a_opcode, 2, 1);
            mipsRegister(a_opcode, 1, 1);
            mipsNibble(a_opcode, 3, 0);
 			//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
          }
          break;
         
          case 0x01:
           pspDebugScreenPuts("moveci   ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
 			//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
         
        case 0x02:
          pspDebugScreenPuts("srl      ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 1, 1);
          mipsNibble(a_opcode, 3, 0);
          //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
          break;

        case 0x03:
          pspDebugScreenPuts("sra      ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 1, 1);
          mipsNibble(a_opcode, 3, 0);
          //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
          break;
          
        case 0x04:
          pspDebugScreenPuts("sllv     ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 1, 1);
          mipsRegister(a_opcode, 0, 0);
          //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
          break;
    
        case 0x06:
          pspDebugScreenPuts("srlv     ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 1, 1);
          mipsRegister(a_opcode, 0, 0);
			//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
          break;
          
         case 0x07:
          pspDebugScreenPuts("srav     ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 1, 1);
          mipsRegister(a_opcode, 0, 0);
			//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
        case 0x08:
          pspDebugScreenPuts("jr       ");
          mipsRegister(a_opcode, 0, 0);
          //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
          break;
          
        case 0x09:
          pspDebugScreenPuts("jalr     ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 0);
			//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
        case 0x0A:
          pspDebugScreenPuts("movz     ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
			//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
        case 0x0b:
          pspDebugScreenPuts("movn     ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
			//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
        case 0x0C:
          pspDebugScreenPuts("syscall  ");
          break;
          
        case 0x0D:
          pspDebugScreenPuts("break    ");
		//
		//mipsimm(a_opcode, 1, 0);
		//
          
			//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CHECK*");
           break;
          
          //0x0e = nothing
          
         case 0x0F:
          pspDebugScreenPuts("sync.p     ");
			//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
        case 0x10:
          pspDebugScreenPuts("mfhi     ");
          mipsRegister(a_opcode, 2, 0);
          //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
        break;

        case 0x11:
          pspDebugScreenPuts("mthi     ");
          mipsRegister(a_opcode, 0, 0);
          //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
        break;
        
        case 0x12:
          pspDebugScreenPuts("mflo     ");
          mipsRegister(a_opcode, 2, 0);
          //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
          break;
          
         case 0x13:
          pspDebugScreenPuts("mtlo     ");
          mipsRegister(a_opcode, 0, 0);
			//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
         
         case 0x14:
          pspDebugScreenPuts("dsllv    ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 1, 1);
          mipsRegister(a_opcode, 0, 0);
			//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
        /* 0x15 unused */  
          
        case 0x16:
          pspDebugScreenPuts("dsrlv    ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 1, 1);
          mipsRegister(a_opcode, 0, 0);
			//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
        
         case 0x17:
          pspDebugScreenPuts("dsrav    ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 1, 1);
          mipsRegister(a_opcode, 0, 0);
			//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
        
        case 0x18:
          pspDebugScreenPuts("mult     ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
			//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
          break;
          
        case 0x19:
          pspDebugScreenPuts("multu    ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
			//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
          break;
          
        case 0x1A:
          pspDebugScreenPuts("div      ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
			//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
          break;
          
        case 0x1B:
          pspDebugScreenPuts("divu     ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
			//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
          break;
          
         case 0x1C:
          pspDebugScreenPuts("dmult    ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
			//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
        case 0x1D:
          pspDebugScreenPuts("dmultu   ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
         case 0x1E:
          pspDebugScreenPuts("ddiv     ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
        case 0x1F:
          pspDebugScreenPuts("ddivu    ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
 		//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
        case 0x20:
          pspDebugScreenPuts("add      ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
          //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
          break;
          
        case 0x21:
          pspDebugScreenPuts("move     ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 0);
          //mipsRegister(a_opcode, 1, 0);
          //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
          break;
          
        case 0x22:
          pspDebugScreenPuts("sub      ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
          //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
          break;
          
        case 0x23:
          pspDebugScreenPuts("subu     ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
          //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
          break;
          
        case 0x24:
          pspDebugScreenPuts("and      ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
          //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
          break;
          
        case 0x25:
          pspDebugScreenPuts("or       ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
          //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
          break;
          
        case 0x26:
          pspDebugScreenPuts("xor      ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
          //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
		break;
          
          case 0x27:
          pspDebugScreenPuts("nor      ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
          //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
          case 0x28:
          pspDebugScreenPuts("mfsa     ");
          mipsRegister(a_opcode, 2, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
          case 0x29:
          pspDebugScreenPuts("msta     ");
          mipsRegister(a_opcode, 0, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
        case 0x2A:
          pspDebugScreenPuts("slt      ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
        break;
          
        case 0x2B:
          pspDebugScreenPuts("sltu     ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
          break;
          
          case 0x2c:
          pspDebugScreenPuts("dadd     ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
          case 0x2d:
          pspDebugScreenPuts("daddu    ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
          case 0x2e:
          pspDebugScreenPuts("dsub     ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
          case 0x2F:
          pspDebugScreenPuts("dsubu    ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 0, 1);
          mipsRegister(a_opcode, 1, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
          case 0x30:
          
          /* 
          	continue here 
          	this register should be 0x00000000 there... lol
                                          ^^     
          */
          
           pspDebugScreenPuts("tge      ");
      mipsRegister(a_opcode, S, 1);
      mipsRegister(a_opcode, T, 1);
      mipsNibble(a_opcode, 3, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
          case 0x31:
           pspDebugScreenPuts("tgeu     ");
      mipsRegister(a_opcode, S, 1);
      mipsRegister(a_opcode, T, 1);
     mipsNibble(a_opcode, 3, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
          case 0x32:
           pspDebugScreenPuts("tlt      ");
      mipsRegister(a_opcode, S, 1);
      mipsRegister(a_opcode, T, 1);
      mipsNibble(a_opcode, 3, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
          case 0x33:
           pspDebugScreenPuts("tltu     ");
      mipsRegister(a_opcode, S, 1);
      mipsRegister(a_opcode, T, 1);
      mipsNibble(a_opcode, 3, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
          case 0x34:
           pspDebugScreenPuts("teq      ");
      mipsRegister(a_opcode, S, 1);
      mipsRegister(a_opcode, T, 1);
      mipsNibble(a_opcode, 3, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
          case 0x36:
           pspDebugScreenPuts("tne      ");
      mipsRegister(a_opcode, S, 1);
      mipsRegister(a_opcode, T, 1);
      mipsNibble(a_opcode, 3, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
          /* 0x37 unused*/
          
          case 0x38:
           pspDebugScreenPuts("dsll     ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 1, 1);
          mipsNibble(a_opcode, 3, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
          /* 0x39 unused */
          
          case 0x3A:
           pspDebugScreenPuts("dsrl     ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 1, 1);
         mipsNibble(a_opcode, 3, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
          case 0x3B:
           pspDebugScreenPuts("dsra     ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 1, 1);
          mipsNibble(a_opcode, 3, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
          case 0x3C:
           pspDebugScreenPuts("dsll32   ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 1, 1);
         mipsNibble(a_opcode, 3, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
          case 0x3E:
           pspDebugScreenPuts("dsrl32   ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 1, 1);
          mipsNibble(a_opcode, 3, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
          case 0x3F:
           pspDebugScreenPuts("dsra32   ");
          mipsRegister(a_opcode, 2, 1);
          mipsRegister(a_opcode, 1, 1);
         mipsNibble(a_opcode, 3, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
        
        default:
           pspDebugScreenPuts("???      ");
      }
      break;
      
    case 0x01:
      switch((a_opcode & 0x1F0000) >> 16)
      {
          case 0x00:
            pspDebugScreenPuts("bltz     ");
            mipsRegister(a_opcode, S, 1);
            mipsImm(a_opcode, 0, 0);
			//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
          break;
            
          case 0x01:
            pspDebugScreenPuts("bgez     ");
            mipsRegister(a_opcode, S, 1);
            mipsImm(a_opcode, 0, 0);
 			//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
          break;
          
           case 0x02:
            pspDebugScreenPuts("bltzl    ");
            mipsRegister(a_opcode, S, 1);
            mipsImm(a_opcode, 0, 0);
            //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
            
           case 0x03:
            pspDebugScreenPuts("bgezl    ");
            mipsRegister(a_opcode, S, 1);
            mipsImm(a_opcode, 0, 0);
            //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
           
           case 0x08:
            pspDebugScreenPuts("tgei     ");
      		mipsRegister(a_opcode, S, 1);
            mipsImm(a_opcode, 0, 0);
            //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
           case 0x09:
            pspDebugScreenPuts("tgeiu    ");
            mipsRegister(a_opcode, S, 1);
            mipsImm(a_opcode, 0, 0);
            //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
           case 0x0A:
            pspDebugScreenPuts("tlti     ");
            mipsRegister(a_opcode, S, 1);
            mipsImm(a_opcode, 0, 0);
            //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
           
           case 0x0B:
            pspDebugScreenPuts("tltiu    ");
            mipsRegister(a_opcode, S, 1);
            mipsImm(a_opcode, 0, 0);
            //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
           
           case 0x0C:
            pspDebugScreenPuts("teqi     ");
            mipsRegister(a_opcode, S, 1);
            mipsImm(a_opcode, 0, 0);
            //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
           
           case 0x0E:
            pspDebugScreenPuts("tnei     ");
            mipsRegister(a_opcode, S, 1);
            mipsImm(a_opcode, 0, 0);
            //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
          
          case 0x10:
            pspDebugScreenPuts("bltzal   ");
            mipsRegister(a_opcode, S, 1);
            mipsImm(a_opcode, 0, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
            break;
            
           case 0x11:
            pspDebugScreenPuts("bgezal   ");
            mipsRegister(a_opcode, S, 1);
            mipsImm(a_opcode, 0, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
            
           case 0x12:
            pspDebugScreenPuts("bltzall  ");
            mipsRegister(a_opcode, S, 1);
            mipsImm(a_opcode, 0, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
             
           case 0x13:
            pspDebugScreenPuts("bgezall  ");
            mipsRegister(a_opcode, S, 1);
            mipsImm(a_opcode, 0, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;  
            
           case 0x18:
            pspDebugScreenPuts("mtsab    ");
            mipsRegister(a_opcode, S, 1);
            mipsImm(a_opcode, 0, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
            
           case 0x19:
            pspDebugScreenPuts("mtsah    ");
            mipsRegister(a_opcode, S, 1);
            mipsImm(a_opcode, 0, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
           break;
            
          default:
            pspDebugScreenPuts("???      ");
      }
      break;
      
    case 0x02:
      pspDebugScreenPuts("j        ");
      mipsImm(a_opcode, 1, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
    case 0x03:
      pspDebugScreenPuts("jal      ");
      mipsImm(a_opcode, 1, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
    case 0x04:
      pspDebugScreenPuts("beq      ");
      mipsRegister(a_opcode, S, 1);
      mipsRegister(a_opcode, T, 1);
      pspDebugScreenPuts("(");
      mipsDec(a_opcode,  0, 0, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
    case 0x05:
      pspDebugScreenPuts("bne      ");
      mipsRegister(a_opcode, S, 1);
      mipsRegister(a_opcode, T, 1);
      pspDebugScreenPuts("(");
      mipsDec(a_opcode,  0, 0, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
    case 0x06:
      pspDebugScreenPuts("blez     ");  
      mipsRegister(a_opcode, S, 1);
      pspDebugScreenPuts("(");
      mipsDec(a_opcode,  0, 0, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
    case 0x07:
      pspDebugScreenPuts("bgtz     ");
      mipsRegister(a_opcode, S, 1);
      pspDebugScreenPuts("(");
      mipsDec(a_opcode,  0, 0, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
    case 0x08:
      pspDebugScreenPuts("addi     ");
      mipsRegister(a_opcode, T, 1);
      mipsRegister(a_opcode, S, 1);
      pspDebugScreenPuts("(");
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
    case 0x09:
      pspDebugScreenPuts("addiu    ");
      mipsRegister(a_opcode, T, 1);
      mipsRegister(a_opcode, S, 1);
      pspDebugScreenPuts("(");
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
    case 0x0A:
      pspDebugScreenPuts("slti     ");
      mipsRegister(a_opcode, T, 1);
      mipsRegister(a_opcode, S, 1);
      pspDebugScreenPuts("(");
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
    case 0x0B:
      pspDebugScreenPuts("sltiu    ");
      mipsRegister(a_opcode, T, 1);
      mipsRegister(a_opcode, S, 1);
      pspDebugScreenPuts("(");
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
    case 0x0C:
      pspDebugScreenPuts("andi     ");
      mipsRegister(a_opcode, T, 1);
      mipsRegister(a_opcode, S, 1);
      mipsImm(a_opcode, 0, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
    case 0x0D:
      pspDebugScreenPuts("ori      ");
      mipsRegister(a_opcode, T, 1);
      mipsRegister(a_opcode, S, 1);
      mipsImm(a_opcode, 0, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
    case 0x0E:
      pspDebugScreenPuts("xori     ");
      mipsRegister(a_opcode, T, 1);
      mipsRegister(a_opcode, S, 1);
      mipsImm(a_opcode, 0, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
    case 0x0F:
      pspDebugScreenPuts("lui      ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
	//these are a bit more granular so these are done in a seperate switch 
	
	case 0x13:
	  pspDebugScreenPuts("cop1x     "); /*there is an inherent flaw in the channeling system of mips imm, we need channels 1 2 3 */
	  mipsImm(a_opcode, 0, 0);
	  //pspDebugScreenSetTextColor(RED); pspDebugScreenPuts("*CHECK*");
	  break;
      
     case 0x14:
      pspDebugScreenPuts("beql     ");
      mipsRegister(a_opcode, S, 1);
      mipsRegister(a_opcode, T, 1);
      mipsDec(a_opcode,  0, 0, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
     break;
      
     case 0x15:
      pspDebugScreenPuts("bnel     ");
      mipsRegister(a_opcode, S, 1);
      mipsRegister(a_opcode, T, 1);
      mipsDec(a_opcode,  0, 0, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
     break;
      
     case 0x16:
      pspDebugScreenPuts("blezl    ");
      mipsRegister(a_opcode, S, 1);;
      mipsDec(a_opcode,  0, 0, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
     break;
     
     case 0x17:
      pspDebugScreenPuts("bgtzl    ");
      mipsRegister(a_opcode, S, 1);
      mipsDec(a_opcode,  0, 0, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
     break;
   
     case 0x18:
      pspDebugScreenPuts("daddi    ");
      mipsRegister(a_opcode, T, 1);
      mipsRegister(a_opcode, S, 1);
      mipsImm(a_opcode, 0, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
     break;
     
     case 0x19:
      pspDebugScreenPuts("daddiu   ");
      mipsRegister(a_opcode, T, 1);
      mipsRegister(a_opcode, S, 1);
      mipsImm(a_opcode, 0, 0);
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
     break;
     
     case 0x1a:
      pspDebugScreenPuts("ldl      ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
     break;
     
     case 0x1b:
      pspDebugScreenPuts("ldr      ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
	//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
     break;
     
     case 0x1c:
      pspDebugScreenPuts("pmfhl    ");
      mipsImm(a_opcode, 0, 0);
     // pspDebugScreenSetTextColor(RED); pspDebugScreenPuts("*CHECK*");
     break;
     
     /*
     case 0x1c:
      pspDebugScreenPuts("mmi      ");
      mipsImm(a_opcode, 1, 0);
      pspDebugScreenSetTextColor(RED); pspDebugScreenPuts("*CHECK*");
     break;
     */
     
      //0x1d is empty
      
     case 0x1e:
      pspDebugScreenPuts("lq       ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
      //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
     break;
     
     case 0x1f:
      pspDebugScreenPuts("sq       ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
      //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
     break;
      
    case 0x20:
      pspDebugScreenPuts("lb       ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;

    case 0x21:
      pspDebugScreenPuts("lh       ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
     case 0x22:
      pspDebugScreenPuts("lwl      ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
    case 0x23:
      pspDebugScreenPuts("lw       ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
     
     case 0x24:
      pspDebugScreenPuts("lbu      ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
      case 0x25:
      pspDebugScreenPuts("lhu      ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
      case 0x26:
      pspDebugScreenPuts("lwr      ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
     case 0x27:
      pspDebugScreenPuts("lwu      ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
    case 0x28:
      pspDebugScreenPuts("sb       ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
     case 0x29:
      pspDebugScreenPuts("sh       ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
      case 0x2A:
      pspDebugScreenPuts("swl      ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
    case 0x2B:
      pspDebugScreenPuts("sw       ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
      case 0x2c:
      pspDebugScreenPuts("sdl      ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
      case 0x2d:
      pspDebugScreenPuts("sdr      ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
      case 0x2e:
      pspDebugScreenPuts("swr      ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
      //important little bastard right here
      case 0x2f:
      pspDebugScreenPuts("cache    ");
      specialRegister(a_opcode, T, 1); //uses special register function
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
//pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
     
      case 0x30:
      pspDebugScreenPuts("ll       ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
      //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
      case 0x31:
      //certified
      pspDebugScreenPuts("lwc1     ");
      floatRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
      //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
      case 0x32:
      pspDebugScreenPuts("lwc2     ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
      //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
      case 0x33:
      pspDebugScreenPuts("pref     ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
      //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
      case 0x34:
      pspDebugScreenPuts("lld      ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
      //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
      case 0x35:
      pspDebugScreenPuts("ldc1     ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
      //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
      /*we are here!*/ 
      
      case 0x36:
      pspDebugScreenPuts("lqc2     ");
      floatRegister(a_opcode, T, 1); /* float */
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
      //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
      case 0x37:
      pspDebugScreenPuts("ld       ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
      //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
      case 0x38:
      pspDebugScreenPuts("sc       ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
      //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
      case 0x39:
      //certified
      pspDebugScreenPuts("swc1     ");
      floatRegister(a_opcode, T, 1); /*float */
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
      //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
      case 0x3a:
      pspDebugScreenPuts("swc2     ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
      //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
      //case 0x3b:
      //break;
      
      case 0x3c:
      pspDebugScreenPuts("sdc      ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
      //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
      case 0x3d:
      pspDebugScreenPuts("sdc1     ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
      //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
     case 0x3e:
      pspDebugScreenPuts("sqc2     ");
      floatRegister(a_opcode, T, 1); /*float */
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
      //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
      
     case 0x3f:
      pspDebugScreenPuts("sd       ");
      mipsRegister(a_opcode, T, 1);
      mipsImm(a_opcode, 0, 0);
      pspDebugScreenPuts("(");
      mipsRegister(a_opcode, S, 0);
      pspDebugScreenPuts(")");
      //pspDebugScreenSetTextColor(GREEN); pspDebugScreenPuts("*CERT*");
      break;
     
    //default:
     // pspDebugScreenPuts("???      ");
  }


  /*
 //unfinished work... better move my ass eh
  switch((a_opcode & 0xFF000000) >> 24)
  {
	case 0x40:
	  pspDebugScreenPuts("mfc0     ");
	  mipsRegister(a_opcode, 1, 1);
	  labelRegister(a_opcode, 2, 0);
	  pspDebugScreenSetTextColor(RED); pspDebugScreenPuts("*CHECK*");
	  break;
	  
	case 0x41:
	  pspDebugScreenPuts("bc0f     ");
	  mipsImm(a_opcode, 1, 0);
	  pspDebugScreenSetTextColor(RED); pspDebugScreenPuts("*CHECK*");
	  break;
	  
	case 0x42:
	  pspDebugScreenPuts("c0       ");
	  mipsImm(a_opcode, 1, 0);
	  pspDebugScreenSetTextColor(RED); pspDebugScreenPuts("*CHECK*");
	  break;
	  
	case 0x43:
	  pspDebugScreenPuts("cop0     "); //broken due to mips imm shit not showing the proper data but it is the proper 24bit width lol
	  mipsImm(a_opcode, 1, 0);
	  pspDebugScreenSetTextColor(RED); pspDebugScreenPuts("*CHECK*");
	  break;  

	case 0x44:
	  pspDebugScreenPuts("mfc1    ");
	  mipsRegister(a_opcode, 1, 1);
	  floatRegister(a_opcode, T, 0); //float
	  pspDebugScreenSetTextColor(RED); pspDebugScreenPuts("*CHECK*");
	  break;
	  
	case 0x45:
	  pspDebugScreenPuts("bc1f    ");
	  mipsImm(a_opcode, 1, 0);
	  pspDebugScreenSetTextColor(RED); pspDebugScreenPuts("*CHECK*");
	  break;

	//case 0x46:
	//  pspDebugScreenPuts("add.s   ");
	//  mipsRegister(a_opcode, 2, 1);
	//  mipsRegister(a_opcode, 1, 1);
	//  mipsRegister(a_opcode, 0, 0);
	//  pspDebugScreenSetTextColor(RED); pspDebugScreenPuts("*CHECK*");
	//  break;

	case 0x46:
	  pspDebugScreenPuts("cop1_s   ");
	  mipsImm(a_opcode, 1, 0);
	  pspDebugScreenSetTextColor(RED); pspDebugScreenPuts("*CHECK*");
	  break;

	case 0x47:
	  pspDebugScreenPuts("cop1   ");
	  mipsImm(a_opcode, 1, 0);
	  pspDebugScreenSetTextColor(RED); pspDebugScreenPuts("*CHECK*");
	  break;

	case 0x48:
	  pspDebugScreenPuts("cop2   ");
	  mipsImm(a_opcode, 1, 0);
	  pspDebugScreenSetTextColor(RED); pspDebugScreenPuts("*CHECK*");
	  break;
			
	case 0x49:
	  pspDebugScreenPuts("bc     ");
	  pspDebugScreenSetTextColor(RED); pspDebugScreenPuts("*CHECK*");
	  break;
			
	case 0x4A:
	  pspDebugScreenPuts("vaddx    ");
	  mipsRegister(a_opcode, 2, 1);
	  mipsRegister(a_opcode, 1, 1);
	  mipsRegister(a_opcode, 0, 0);
	  pspDebugScreenSetTextColor(RED); pspDebugScreenPuts("*CHECK*");
	  break;
		
	case 0x4B:
	  pspDebugScreenPuts("vaddx.x  ");
	  mipsRegister(a_opcode, 2, 1);
	  mipsRegister(a_opcode, 1, 1);
	  mipsRegister(a_opcode, 0, 0);
	  pspDebugScreenSetTextColor(RED); pspDebugScreenPuts("*CHECK*");
	  break;
	  
  }

  */

  pspDebugScreenSetTextColor(WHITE);
}
