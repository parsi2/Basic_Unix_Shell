#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXLINE 100
enum virtual_commands{WRT, RD, sMain, sDisk, sTable};
int FIFOC = 0;
int LRUC = 0;

struct PageTable
{
	int vpgn;
    int isValid;
    int dirtynum;
    int pagenum;
    int beingused;
};

int pageFaultH(struct PageTable* pagetable, int* mainMem, int* diskMem, int FIFO, int* list)
{
    int j, k;
	int getp = 0;
    int UsedMain[4] = {0,0,0,0};

    for(j = 0; j < 8; j++)
    {
        if(pagetable[j].isValid == 1) { UsedMain[pagetable[j].pagenum] = 1; }
    }

    for(j = 0; j < 4; j++)
    {
        if(UsedMain[j] == 0) { return j; }
    }

    if(FIFO == 1)
    {
		int temp = list[0];

		for(j = 0; j < 3; j++) { list[j] = list[j++]; }
		list[3] = -1;
		FIFOC--;

		for(j = 0; j < 8; j++)
		{
			if(pagetable[j].isValid == 1)
			{
				if(pagetable[j].pagenum == getp)
				{
					if(pagetable[j].dirtynum == 1)
					{
						for(k = 0; k < 8; k++)
						{
							//swap
							diskMem[(8*pagetable[j].vpgn)+k] = mainMem[(8*pagetable[j].pagenum)+k];
							mainMem[(8*pagetable[j].pagenum)+k] = -1;
						}
					}

					pagetable[j].dirtynum = 0;
					pagetable[j].pagenum = j;
					pagetable[j].isValid = 0;
				}
			}
		}

        return getp;
    }
    else //get LRU
    {
		int lowestp = 12000;
		for(j = 0; j < 8; j++)
		{
			//check if valid bit is 1 and check if the pagetable page is the smalled priority
			if(pagetable[j].isValid == 1)
			{
				if(pagetable[j].beingused < lowestp)
				{
					getp = pagetable[j].pagenum;
					lowestp = pagetable[j].beingused;
				}
			}
		}

		for(j = 0; j < 8; j++)
		{
			if(pagetable[j].isValid == 1)
			{
				if(pagetable[j].pagenum == getp)
				{
					if(pagetable[j].dirtynum == 1)
					{
						for(k = 0; k < 8; k++)
						{
							//swap
							diskMem[(8*pagetable[j].vpgn)+k] = mainMem[(8*pagetable[j].pagenum)+k];
							mainMem[(8*pagetable[j].pagenum)+k] = -1;
						}
					}

					pagetable[j].dirtynum = 0;
					pagetable[j].pagenum = j;
					pagetable[j].isValid = 0;
				}
			}
		}

        return getp;
    }
}

int commands(char* buffer)
{
    if(strcmp(buffer, "write") == 0) { return WRT; }
    if(strcmp(buffer, "read") == 0) { return RD; }
    if(strcmp(buffer, "showdisk") == 0) { return sDisk; }
    if(strcmp(buffer, "showmain") == 0) { return sMain; }
    return sTable;

}

int parser(char buf[], char** argv)
{
    char* parse;
    parse = strtok(buf, " ");
    int l = 0;

    while(parse)
    {
        argv[l++] = parse;
        parse = strtok(NULL, " ");
    }
    return l;
}

void showptable(struct PageTable pagetable[])
{
    int l;
    for(l = 0; l < 8; l++)
    {
        printf("%d:%d:%d:%d\n", l, pagetable[l].isValid, pagetable[l].dirtynum, pagetable[l].pagenum);
    }
}

void showMethod(int* memory, int pagenum)
{
    int pg = pagenum*8;
    int i = 8;
    while(i > 0){
        printf("%d:%d\n", pg, memory[pg]);
        i--;
        pg++;
    }
}

int PhysicalAddressNum(struct PageTable* pagetable, int* mainMem, int viraddr)
{
    int offs = viraddr % 8;
    return ((pagetable[viraddr/8].pagenum*8) + offs);
}

int main(int argc, char **argv)
{
    int i, j, k;
    int FIFO = 1;
    char buf[MAXLINE];
    char* argv2[MAXLINE];
    int counter;
    int viraddr;
    int vir_pgnum;
    int physicaladdr;
    int mainMem[32];
    int diskMem[64];
    int list[4];

    for(i = 0; i < 4; i++) { list[i] = -1; }

    if (argc > 1 && strcmp(argv[1], "LRU") == 0)
    {
		FIFO = 0;
	}

    struct PageTable pagetable[8];

    for(i = 0; i < 8; i++)
    {
		pagetable[i].vpgn = i;
        pagetable[i].isValid = 0;
        pagetable[i].dirtynum = 0;
        pagetable[i].pagenum = i;
        pagetable[i].beingused = 0;
    }

    for(j = 0; j < 32; j++) { mainMem[j] = -1; }
    for(k = 0; k < 64; k++) { diskMem[k] = -1; }
    

    while(1) {
        printf("> ");
        if(fgets(buf, MAXLINE, stdin) == NULL) exit(0);
        size_t len = strlen(buf);
        if(buf[len - 1] == '\n')
            buf[len - 1] = '\0';

        if(strcmp(buf, "quit") == 0) break;
        if(strcmp(buf, "showptable") == 0) commands(buf);
        if(buf[0] == NULL) continue;
        
        counter = parser(buf, argv2);

        if(counter > 0)
        {
            viraddr = atoi(argv2[1]);
            vir_pgnum = atoi(argv2[1])/8;
        }

        switch(commands(argv2[0]))
        {
            case RD:
                if(pagetable[vir_pgnum].isValid == 1) {
					pagetable[vir_pgnum].beingused = LRUC;
					LRUC++;
					physicaladdr = PhysicalAddressNum(pagetable, mainMem, viraddr);
					printf("%d\n", mainMem[physicaladdr]);
				}
                else
                {
                    pagetable[vir_pgnum].pagenum = pageFaultH(pagetable, mainMem, diskMem, FIFO, list);
                    pagetable[vir_pgnum].isValid = 1;
					physicaladdr = PhysicalAddressNum(pagetable, mainMem, viraddr);
					list[FIFOC] = pagetable[vir_pgnum].pagenum;
                    //printf("Address change %d to %d\n", viraddr, physicaladdr);
					pagetable[vir_pgnum].beingused = LRUC;
					LRUC++;
					FIFOC++;
                    printf("A Page Fault Has Occurred\n");
                    printf("%d\n", mainMem[physicaladdr]);
                }
                break;
            
            case WRT:
                //printf("Address change %d to %d\n", viraddr, physicaladdr);
                if(pagetable[vir_pgnum].isValid == 1)
                {
                    pagetable[vir_pgnum].dirtynum = 1;
                    physicaladdr = PhysicalAddressNum(pagetable, mainMem, viraddr);
                    mainMem[physicaladdr] = atoi(argv2[2]);
                    //pagetable[vir_pgnum].pagenum = physicaladdr/8;
					list[FIFOC] = pagetable[vir_pgnum].pagenum;
					pagetable[vir_pgnum].beingused = LRUC;
					FIFOC++;
					LRUC++;

                }
                else
                {
                    pagetable[vir_pgnum].pagenum = pageFaultH(pagetable, mainMem, diskMem, FIFO, list);
                    pagetable[vir_pgnum].isValid = 1;
                    pagetable[vir_pgnum].dirtynum = 1;
                    physicaladdr = PhysicalAddressNum(pagetable, mainMem, viraddr);
					mainMem[physicaladdr] = atoi(argv2[2]);
					list[FIFOC] = pagetable[vir_pgnum].pagenum;
					pagetable[vir_pgnum].beingused = LRUC;
					LRUC++;
					FIFOC++;
                    printf("A Page Fault Has Occurred\n");
                }
                break;
            
            case sMain:
                showMethod(mainMem, atoi(argv2[1]));
                break;
            
            case sDisk:
                showMethod(diskMem, atoi(argv2[1]));
                break;
            case sTable:
                showptable(pagetable);
                break;

        }
    }
    return 0;
}
