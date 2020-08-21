#include <stdio.h>

#define BUFF_MAX_LENGTH 32767
#define TRUE 1
#define FALSE 0

void main(int par_count, char *par[])
{
  unsigned char buffer[BUFF_MAX_LENGTH];

  if(par_count<4)
  {
    printf("Debe especificar un nombre de archivo de origen, uno de destino y un nombre para la variable");
    return;
  }

  FILE *origin = fopen(par[1],"rb");
  FILE *destiny = fopen(par[2],"w");

  unsigned int i=0;
  while(1)
  {
    unsigned char byte = fgetc(origin);
    if(feof(origin) || i>BUFF_MAX_LENGTH) break;
    buffer[i]=byte;
    i++;
  }

  fprintf(destiny,"unsigned char %s[%u] = {\n", par[3],i);
  unsigned int l=0;
  unsigned char flag=TRUE;
  for(int j=0;j<i;j+=8)
  {
    fprintf(destiny,"\t\t");
    for(int k=0;k<16;k++)
    {
      fprintf(destiny,"0x%X, ",buffer[l]);
      l++;
      if(l>=i) flag=FALSE;
      if(!flag) break;
    }
    fprintf(destiny,"\n");
    if(!flag) break;
  }
  fprintf(destiny,"};");
  fclose(origin);
  fclose(destiny);
}
