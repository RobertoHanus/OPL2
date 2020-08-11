#include <stdio.h>
#include <dos.h>

// I prefer using these Assembler-like types
typedef unsigned int  WORD;
typedef unsigned char BYTE;

/*
* FM Synthesizer base port. SB Pro II - 0x220, Adlib 0x388
*/

WORD FMport = 0x388;

/*
* Enables OPL3 extensions.
*/

WORD OPL3 = 0;

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


void main()
{

   for(int i=0x01;i<=0xF5;i++)
   {
      //FMwriteReg(i,0x00);
   }


   FMwriteReg(0x20,0x02);
   FMwriteReg(0x40,0x10);
   FMwriteReg(0x60,0xF0);
   FMwriteReg(0x80,0x77);
   FMwriteReg(0xA0,0x98);
   FMwriteReg(0x23,0x01);
   FMwriteReg(0x43,0x00);
   FMwriteReg(0x63,0xF0);
   FMwriteReg(0x83,0x77);
   FMwriteReg(0xB0,0x31);


   FMwriteReg(0x41,0x10);
   FMwriteReg(0x61,0xF0);
   FMwriteReg(0x81,0x77);
   FMwriteReg(0xA1,0xb0);
   FMwriteReg(0x24,0x01);
   FMwriteReg(0x44,0x00);
   FMwriteReg(0x64,0xF0);
   FMwriteReg(0x84,0x77);
   FMwriteReg(0xB1,0x31);








   delay(1000);

   FMwriteReg(0xB0,0x11);
   FMwriteReg(0xB1,0x11);
}
