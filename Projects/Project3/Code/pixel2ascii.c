#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>


#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include "pixel2ascii.h"

#define READ_BUFFER_LEN 16
//start PPM tools functions
 char* readType(FILE *fp)
 {
	char buf[READ_BUFFER_LEN];
	char *t; 
    
    //get line of the file into read buffer -> type of ppm
    t = fgets(buf, READ_BUFFER_LEN, fp);
    //check if it reads something and if it is a P6 type
    if (t == NULL)
    {
		printf("could not read type from file\n");
		return "";
	}
	
	if(strncmp(buf, "P6\n", 3) != 0 )
	{
		printf("type is not P6\n");
		return "";
	}
	else
	{
		//printf("set type as P6\n");
		//set type as P6
		return "P6";
	}    
}

unsigned int readNextHeaderValue(FILE *fp)
{
	char buf[READ_BUFFER_LEN];
	char *t; 
	int r;
	int value;
	
	//printf("read next header value\n");
    //load next line
    t = fgets(buf, READ_BUFFER_LEN, fp);
    if(t == NULL)
    {
		printf("Error reading from file");
		return 0;
	}
	
	//read value as unsigned int
    r = sscanf(buf, "%u", &value);
    
    //check if the read was successful
    if ( r != 1 ) 
    {
		printf("Could not read width value\n");
		return 0;
	}
	else
	{
		return value;
	}
}
        
 
header* getImageHeader(FILE *fp)
{
	//alloc header
	header* h = malloc(sizeof(header));  
	
	//check if file pointer is null
    if (fp == NULL)
    {
		//file pointer is null
		return NULL;
	}
	
	//get type
	char* type = readType(fp);
	if(strcmp(type, "") != 0)
	{
		//write type
		strcpy(h->type,type);
	}
	else
	{
		//error reading type
		return NULL;
	}
    
    h->width = readNextHeaderValue(fp);
    h->height = readNextHeaderValue(fp);
    h->depth = readNextHeaderValue(fp);
    
    //check for successful read and valid color range
	if(h->depth != 255 || h->width == 0 || h->height == 0)
	{
		//printf("error reading values\n");
		return NULL;
	}
	
	return h;
}
//end PPM tools functions

//darker to lighter
char charList[ASCII_LEVELS] = {'#','W','K','D','G','L','f','t','j','i','+',';',':',',','.',' '};


char getCharForGray(int g)
{
	int index = g * (ASCII_LEVELS-1) / GRAY_LEVELS;
	//printf("char index: %d\n",index);
	return charList[index];
}


char* pixel2ASCII(pixel* image, int w, int h, int numHorizontalSquares, int numVerticalSquares)
{
	//int final_h = calcHeight(w,h,final_w);

	//calc the w and h of each sample rect
	int rectW = w/numHorizontalSquares;
	int rectH = h/numVerticalSquares;

	
	//printf("rectW: %d\nrectH: %d\nw: %d\nh: %d\n",rectW,rectH,numHorizontalSquares,numVerticalSquares);
	
	//alocate memory for char array
	char* ascii = malloc(numHorizontalSquares*numVerticalSquares*sizeof(char));
	
	
	int i,j,n,m;
	//for each square
	for(i=0; i < numVerticalSquares; i++)
	{
		for(j=0; j< numHorizontalSquares; j++)
		{
			int sum = 0;
			//for each pixel in square
			for(n=rectH*i; n < rectH*(i+1); n++)
			{
				for(m=rectW*j; m< rectW*(j+1); m++)
				{
					pixel p = image[n*w + m];
					sum += (p.red + p.green + p.blue) / 3;
				}
			}
			
			sum = sum/(rectW*rectH);
			
			//printf("found average: %d\n",sum);
			char c = getCharForGray(sum);
			//printf("found char: %c\n",c);
			
			int index = i*numHorizontalSquares + j;
			//printf("index: %d\n",index); 
			ascii[index] = c;
		}
	}
	
	return ascii;
}

//Sets the cursor in the console to any point given
void setPointer(int x, int y)
{
    printf("\033[%d;%dH", y, x);
}

void clearConsole()
{
	printf("\033c");
}

//prints the image in the screen
void printAsciiImage(char* asciiImage, int col)
{
    int i;
    for (i = 0; i < strlen(asciiImage); i++)
	{
		printf("%c",asciiImage[i]);
	
		if(i%col==0)
			printf("\n");
	}
	//final newline
	printf("\n");
}


