#include <stdio.h>
#include <string.h>
#include <dos.h>
#include <math.h>

#define EQUAL 0
#define NOT_EQUAL -1
#define NOT_FOUND -1
#define TRUE 1
#define FALSE 0

// I prefer using these Assembler-like types
typedef unsigned int  WORD;
typedef unsigned char BYTE;

/*
* FM Synthesizer base port. SB Pro II - 0x220, Adlib 0x388
*/

WORD FMport = 0x388;

/*
* Direct write to any Adlib/SB Pro II FM synthetiser register.
*   reg - register number (range 0x001-0x0F5 and 0x101-0x1F5). When high byte
*         of reg is zero, data go to port FMport, otherwise to FMport+2
*   data - register value to be written
*/
BYTE FMwriteReg(WORD reg, BYTE data)
{
   asm {
      mov     dx,FMport
      mov     ax,reg
      or      ah,ah           // high byte is nonzero -- write to port base+2
      jz      out1
      inc     dx
      inc     dx
   }
   out1: asm {
      out     dx,al
      mov     cx,6
   }
   loop1:asm {                     // delay between writes
      in      al,dx
      loop    loop1
      inc     dx
      mov     al,data
      out     dx,al
      dec     dx
      mov     cx,36
   }
   loop2:asm {                     // delay after data write
      in      al,dx
      loop    loop2
   }
      return _AL;
}

char notes_universe[12][3]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

int notes_freq[12]={ 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494 };

unsigned char op1_offset[9] = { 0x00, 0x01, 0x02, 0x08, 0x09, 0x0A, 0x10, 0x11, 0x12 };
unsigned char op2_offset[9] = { 0x03, 0x04, 0x05, 0x0B, 0x0C, 0x0D, 0x13, 0x14, 0x15 };

unsigned char op1_reg_for_channel(unsigned char reg, unsigned char channel)
{
  return op1_offset[channel]+reg;
}

unsigned char op2_reg_for_channel(unsigned char reg, unsigned char channel)
{
  return op2_offset[channel]+reg;
}

int note_num(char *note_name)
{
   for(int i=0;i<12;i++)
   {
      int result=strcmp(notes_universe[i],note_name);
      if(result==0) return i;
   }
   return NOT_FOUND;
}

int fnum(char *note, unsigned char oct)
{
  int freq = notes_freq[note_num(note)];
  return freq*pow(2,20-oct)/49716;
}

void FMnote_on(unsigned char *note, unsigned char oct, unsigned char channel)
{
  int fnumber = fnum(note,oct);
  int LSB_fnum = fnumber & 0xFF;
  FMwriteReg(0xA0+channel,LSB_fnum);
  FMwriteReg(0xB0+channel,0x31);
}

void FMnote_off(unsigned char channel)
{
  FMwriteReg(0xB0+channel,0x11);
}

void FMInit()
{
  for(int i=0x01;i<=0xF5;i++) FMwriteReg(i,0x00);

  for(int j=0;j<9;j++)
  {
    FMwriteReg(op1_reg_for_channel(0x20,j),0x01);
    FMwriteReg(op1_reg_for_channel(0x40,j),0x10);
    FMwriteReg(op1_reg_for_channel(0x60,j),0xF0);
    FMwriteReg(op1_reg_for_channel(0x80,j),0x77);

    FMwriteReg(op2_reg_for_channel(0x20,j),0x01);
    FMwriteReg(op2_reg_for_channel(0x40,j),0x10);
    FMwriteReg(op2_reg_for_channel(0x60,j),0xF0);
    FMwriteReg(op2_reg_for_channel(0x80,j),0x77);
  }
}

void main()
{
  FMInit();
  FMnote_on("C",4,0);
  delay(1000);

  FMnote_on("D",4,1);

  delay(1000);

  FMnote_off(0);
  FMnote_off(1);
}
