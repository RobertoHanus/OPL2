#include <stdio.h>
#include <string.h>
#include <dos.h>
#include <math.h>

#define EQUAL 0
#define NOT_EQUAL -1
#define NOT_FOUND -1
#define TRUE 1
#define FALSE 0
#define ON 1
#define OFF 0

char notes_universe[12][3]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

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

int fnum(char *note)
{
  double freq = notes_freq[note_num(note)];
  return freq*pow(2,16)/49716;
}

void FMnote_on(unsigned char *note, unsigned char oct, unsigned char channel)
{
  int fnumber = fnum(note);
  int LSB_fnum = fnumber & 0xFF;
  int MSB_fnum = (fnumber >> 8) & 0x03;

  FMwriteReg(0xA0+channel,LSB_fnum);
  FMwriteReg(0xB0+channel,MSB_fnum | (oct<<2) | 0x20);
}

void FMnote_off(char *note, unsigned char oct, unsigned char channel)
{
  int fnumber = fnum(note);
  int MSB_fnum = (fnumber >> 8) & 0x03;
  FMwriteReg(0xB0+channel,MSB_fnum | (oct<<2));
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
    FMwriteReg(op2_reg_for_channel(0x40,j),0x00);
    FMwriteReg(op2_reg_for_channel(0x60,j),0xF0);
    FMwriteReg(op2_reg_for_channel(0x80,j),0x77);
  }
}

struct note_register {
  char *note;
  unsigned char oct;
  unsigned char on;
};

note_register channel[9];

void play_note(char *note, unsigned char oct)
{
  for(int i=0;i<9;i++)
  {
    if(channel[i].on == OFF)
    {
      channel[i].note=note;
      channel[i].oct=oct;
      channel[i].on=ON;
      FMnote_on(note,oct,i);
      break;
    }
  }
}

void mute_note(char *note, unsigned char oct)
{
  for(int i=0;i<9;i++)
  {
    if(*channel[i].note == *note && channel[i].oct == oct && channel[i].on == ON)
    {
      channel[i].on=OFF;
      FMnote_off(note, oct, i);
      break;
    }
  }
}

char *note_name(int note_num)
{
   return notes_universe[note_num];
}


int note_freq(int note_num, int note_oct)
{
	int freq[][11] = {
	{16,33,65,131,262,523,1047,2093,4186,8372,16744},
	{17,35,69,139,277,554,1109,2217,4435,8867,17740},
	{18,37,73,147,294,587,1175,2349,4699,9397,18795},
	{19,39,78,156,311,622,1245,2489,4978,9956,19912},
	{21,41,82,165,330,659,1319,2637,5274,10548,0},
	{22,44,87,175,349,698,1397,2794,5588,11175,0},
	{23,46,93,185,370,740,1480,2960,5920,11840,0},
	{25,49,98,196,392,784,1568,3136,6272,12544,0},
	{26,52,104,208,415,831,1661,3322,6645,13290,0},
	{28,55,110,220,440,880,1760,3520,7040,14080,0},
	{29,58,117,233,466,932,1865,3729,7459,14917,0},
	{31,62,123,247,494,988,1976,3951,7902,15804,0}
			};

	return freq[note_num][note_oct];
}

void note_on(char *note_name, int note_oct)
{
   sound(note_freq(note_num(note_name),note_oct));
   return;
}

void note_off()
{
   nosound();
}

struct full_note_des {
   char *name;
   int oct;
};

full_note_des midi_note(int midi_note_num)
{
   full_note_des note;

   int note_oct = midi_note_num / 12;
   int note_num = midi_note_num - note_oct * 12;

   note.name = note_name(note_num);
   note.oct = note_oct;

   return note;
}

void midi_note_on(int midi_note_num)
{
    full_note_des note=midi_note(midi_note_num);
    note_on(note.name,note.oct);
}

void FMmidi_note_on(int midi_note_num)
{
    full_note_des note=midi_note(midi_note_num);
    play_note(note.name,note.oct);
}

void FMmidi_note_off(int midi_note_num)
{
    full_note_des note=midi_note(midi_note_num);
    mute_note(note.name,note.oct);
}

struct midi_header_chunk{
   char name[4];
   unsigned char length[4]; //Little endian by default
   unsigned char format[2]; //Little endian by default
   unsigned char tracks[2]; //Little endian by default
   unsigned char ticks[2];  //Little endian by default
};

void little_endian_to_big_endian(unsigned char *p, int length)
{
   int last_byte=length-1;
   for(int i=0;i<length/2;i++)
   {
      unsigned char tmp;
      tmp=p[i];
      p[i]=p[last_byte-i];
      p[last_byte-i]=tmp;
   }
}

struct midi_track_header_chunk {
   char name[4];
   unsigned char length[4];  //Little endian by default
};

void main(int args_count, char *args[])
{
   if(args_count<2)
   {
      printf("Debe ingresar un nombre de archivo midi");
   }

   unsigned char midi_events[32768];
   FILE *stream = fopen(args[1],"rb");
   if(stream==NULL)
   {
      printf("Can't open midi file");
      return;
   }

   midi_header_chunk header;

   fread(&header,sizeof(midi_header_chunk),1,stream);

   little_endian_to_big_endian(header.length,sizeof(header.length));
   little_endian_to_big_endian(header.format,sizeof(header.format));
   little_endian_to_big_endian(header.tracks,sizeof(header.tracks));
   little_endian_to_big_endian(header.ticks,sizeof(header.ticks));

   midi_track_header_chunk track_header;

   fread(&track_header,sizeof(midi_track_header_chunk),1,stream);

   little_endian_to_big_endian(track_header.length,sizeof(track_header.length));

   int midi_events_length;
   if(*((long int*)track_header.length)>32768)
      midi_events_length=32768;
   else
      midi_events_length=*((long int*)track_header.length);

   fread(midi_events,midi_events_length,1,stream);

   unsigned char delta_time[4];
   unsigned char delta_data[4];

   unsigned char tempo[4];
   unsigned long u_seconds_per_ticks=0;
   unsigned long int *next_event=(unsigned long int *)delta_time;

   unsigned char last_note;

   int i=0;

   do
   {
      int j=0;
      *((unsigned long int *)delta_data)=0l;
      *((unsigned long int *)delta_time)=0l;
      while(1)
      {
	 if((midi_events[i] & 0x80) == 0)
	 {
	    delta_data[j] = midi_events[i];
	    i++;
	    j++;
	    break;
	 }
	 else
	 {
	    delta_data[j] = midi_events[i];
	 }
	 i++;
	 j++;

	 if(j>4)
	 {
	    printf("\nError: delta time too big");
	    return;
	 }
      }


      switch(j)
      {
	 case 1:
	    delta_time[0] = delta_data[0] & 0x7F;
	    break;
	 case 2:
	    little_endian_to_big_endian(delta_data,j);
	    delta_time[0] = delta_data[0] & 0x7F;
	    delta_time[0] = delta_time[0] | delta_data[1] << 7;
	    delta_time[1] = delta_data[1] & 0x7F;
	    delta_time[1] = delta_time[1] >> 1;
	    break;
	 case 3:
	    little_endian_to_big_endian(delta_data,j);
	    delta_time[0] = delta_data[0] & 0x7F;
	    delta_time[0] = delta_time[0] | delta_data[1] << 7;
	    delta_time[1] = delta_data[1] & 0x7F;
	    delta_time[1] = delta_time[1] >> 1;
	    delta_time[1] = delta_time[1] | delta_data[2] << 6;
	    delta_time[2] = delta_data[2] & 0x7F;
	    delta_time[2] = delta_time[2] >> 2;
	    break;
	 case 4:
	    little_endian_to_big_endian(delta_data,j);
	    delta_time[0] = delta_data[0] & 0x7F;
	    delta_time[0] = delta_time[0] | delta_data[1] << 7;
	    delta_time[1] = delta_data[1] & 0x7F;
	    delta_time[1] = delta_time[1] >> 1;
	    delta_time[1] = delta_time[1] | delta_data[2] << 6;
	    delta_time[2] = delta_data[2] & 0x7F;
	    delta_time[2] = delta_time[2] >> 2;
	    delta_time[2] = delta_time[2] | delta_data[3] << 5;
	    delta_time[3] = delta_data[3] & 0x7F;
	    delta_time[3] = delta_time[3] >> 3;
	    break;
      }



      printf("\n%lX",*((unsigned long int*)delta_time));



      // Midi meta events
      switch(midi_events[i])
      {
	 case 0xFF:
	 {
	    i++;
	    switch(midi_events[i])
	    {
	       case 0x00:
	       {
		  i++;
		  int sequence_length=(int)midi_events[i];
		  i+= sequence_length+1;
		  break;
	       }

	       case 0x01:
	       {
		  i++;
		  int text_length=(int)midi_events[i];
		  i+= text_length+1;
		  break;
	       }
	       case 0x02:
	       {
		  i++;
		  int text_length=(int)midi_events[i];
		  i+= text_length+1;
		  break;
	       }
	       case 0x03:
	       {
		  i++;
		  int text_length=(int)midi_events[i];
		  i+= text_length+1;
		  break;
	       }
	       case 0x04:
		  i++;
		  int text_length=(int)midi_events[i];
		  i+= text_length+1;
		  break;
	       case 0x05:
	       {
		  i++;
		  int text_length=(int)midi_events[i];
		  i+= text_length+1;
		  break;
	       }
	       case 0x06:
	       {
		  i++;
		  int text_length=(int)midi_events[i];
		  i+= text_length+1;
		  break;
	       }
	       case 0x07:
	       {
		  i++;
		  int text_length=(int)midi_events[i];
		  i+= text_length+1;
		  break;
	       }
	       case 0x58:
		  i+=6;
		  break;
	       case 0x51:
		  i++;
		  tempo[3]=0;
		  i++;
		  tempo[2]=midi_events[i];
		  i++;
		  tempo[1]=midi_events[i];
		  i++;
		  tempo[0]=midi_events[i];
		  i++;
		  u_seconds_per_ticks=(*((unsigned long int*)tempo))/(*((unsigned int *)header.ticks));
		  break;
	       case 0x59:
		  i+=4;
		  break;
	       case 0x7F:
		  i++;
		  int data_length=(int)midi_events[i];
		  i+= data_length+1;
		  break;
	       case 0x2F:
		  fclose(stream);
		  return;
	    }
	    continue;
	 }
      }



      delay(((*next_event) * u_seconds_per_ticks) / 1000);


      // Midi events commands
      switch(midi_events[i]>>4)
      {
	 case 0x9:
	    i++;
	    FMmidi_note_on(midi_events[i]);
	    i+=2;
	    break;
	 case 0x8:
	    i++;
	    FMmidi_note_off(midi_events[i]);
	    i+=2;
	    break;
	 case 0xA:
	    i++;
	    i+=2;
	    break;
	 case 0xB:
	    i++;
	    i+=2;
	    break;
	 case 0xC:
	    i++;
	    i++;
	    break;
	 case 0xD:
	    i++;
	    i++;
	    break;
	 case 0xE:
	    i++;
	    i+=2;
	    break;
      }


   } while(i < midi_events_length);
   fclose(stream);
}
