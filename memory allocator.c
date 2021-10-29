#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAXLINE 256
#define MAXHEAP 127
#define SPACEADD 126
#define HDFT 2
#define SHIFT 1
unsigned char theHeap[MAXHEAP];

void imp_malloc(int bytes)
{
    int newHeap;
    int newBytes, newB, newBY;
    int j = 0;
    int k = 0;
    while(j < SPACEADD)
    {
        newHeap = theHeap[j] & 1; //check to see if allocated
        if(newHeap != 1)
        {
            newBytes = HDFT + bytes;
            theHeap[j] = (newBytes << 1);
            theHeap[j] += SHIFT;
            int newJ = j+SHIFT; // allocated header
            printf("%d\n", newJ);

            j += (SHIFT+bytes);
            //shift bytes
            theHeap[j] = newBytes << SHIFT;
            theHeap[j] += SHIFT; //allocated footer

            j++; //new free block
            if(theHeap[j] == 0)
            {
                k = (SHIFT+j);
                while(k < MAXHEAP && theHeap[k]==0 ) { k++; }

                newBY = k-j;
                --newBY;
                newB = HDFT+newBY;
                theHeap[j] = (newB<<SHIFT);
                j += (SHIFT+newBY);
                theHeap[j] = (newB<<SHIFT);
            }
            //since no new heap space
            return;
        }
        //new allocated block
        j += (theHeap[j] >> SHIFT);
        //no longer need since we are adding the header footer in byte size
        // j = j+2
    }

}

void imp_free(int bytes)
{
    int b = bytes-SHIFT;
    int newBytes = (theHeap[b] >> 1)-HDFT; //this will produce the bytes space
    int j, k, h, newHeap;
    for(j = bytes; j < bytes + newBytes; j++) { theHeap[j] = 0; } // initializing to empty (0)
    //check for header and footer
    theHeap[b] &= -HDFT;
    theHeap[j] &= -HDFT;
    k = 0; //reset k and j
    j = 0;
    while(j < SPACEADD) {
        newHeap = theHeap[j] & SHIFT; //check to see if allocated
        if(newHeap != 1)
        {
            k = j + (theHeap[j]>>SHIFT);

            if(SPACEADD <= k) { break; }
            k += (theHeap[k]>>SHIFT);
            //shift k
            k -= SHIFT;

            newHeap = theHeap[k] & SHIFT; //check to see if allocated
            if(newHeap != 1)
            {
                //add new block
                for(h = 1+j; h < k; h++) { theHeap[h] = 0; }
                h -= j;
                //shift h
                h = h + SHIFT;
                theHeap[j] = h<<SHIFT;
                theHeap[k] = h<<SHIFT;
                continue;
            }

        }
        //new allocated block
        j += (theHeap[j] >> SHIFT);
        //new allocated block included with header and footer
        //j+=2;
    }
}

void printmem(int location, int characters)
{
    // need to loop through and print the number of characters
    int j;
    for(j = 0; j < characters; j++) {
        // print hexadecimal format 
        int newLocation = location + j;
        printf("%X ", theHeap[newLocation]);
    }
    // print new line
    printf("\n"); 
}

void writemem(int location, char* str)
{
    // need to loop through
    int j;
    int newStr = strlen(str);
    for(j = 0; j < newStr; j++) {
        int newLocation = location + j;
        //need to write to new memory
        theHeap[newLocation] = str[j];
    }
}

void blocklist()
{
    int newHeap;
    int j = 0;
    while(j < SPACEADD)
    {
        //if(j >= SPACEADD) return;
        //need to print in the order start of payload, payload size and allocation status
        printf("%d, %d, %-10s\n", 1+j, (theHeap[j]>>1)-2, ((theHeap[j]&1)==1) ? "allocated." : "free.");
        j += (theHeap[j]>>SHIFT);
        //j = 2+j;
    }
}

int main(int argc, char **argv)
{
    int i;
    unsigned int space = 125;

    memcpy(&theHeap[0], &space, 1);
    theHeap[0] = theHeap[0]<<1;
    memcpy(&theHeap[SPACEADD], &space, 1);
    theHeap[SPACEADD] = theHeap[SPACEADD]<<1;
    //initialize the heap
    for(i = 1; i < SPACEADD; i++)
        theHeap[i] = 0;

    char buf[MAXLINE];
    while(1) {
        printf("> ");
        if(fgets(buf, MAXLINE, stdin) == NULL) exit(0);
        size_t len = strlen(buf);
        if(buf[len - 1] == '\n')
        buf[len - 1] = '\0';
        if(strcmp(buf, "quit") == 0) break;
        if(strcmp(buf, "blocklist\0") == 0)
        {
            blocklist(); 
            continue;
        }
        if(buf[0] == NULL) continue;

        char* parse;
        parse = strtok(buf, " ");
        int l = 0;
        while(parse != NULL)
        {
            argv[l] = parse;
            parse = strtok(NULL, " ");
            l++;
        }
        argv[l] = NULL;
        argc = l;
        int getInt = atoi(argv[1]);
        if(strcmp(*argv, "malloc") == 0) { imp_malloc(getInt); }
        else if(strcmp(*argv, "free") == 0) { imp_free(getInt); }
        else if(strcmp(*argv, "writemem") == 0) { writemem(getInt, argv[2]); }
        else if(strcmp(*argv, "printmem") == 0) { printmem(getInt, atoi(argv[2])); }
    }
}