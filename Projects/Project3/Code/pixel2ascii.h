#define GRAY_LEVELS 255
#define ASCII_LEVELS 16

//this is a convinient typedef to better read the code.
//it defines "byte" as a unsigned 8bit int.
typedef u_int8_t  byte;

//this struct represents each pixel.
//should be used to handle each pixel and maintain their correct colors.
typedef struct pixel_t {
	byte red;
	byte green;
	byte blue;
}pixel;

//this struct represents the ppm file header.
//it stores the file info and will be copied to the output file.
typedef struct header_t {
	char type[3];
	unsigned int width;
	unsigned int height;
	unsigned int depth;
}header;

char getCharForGray(int g);
char* pixel2ASCII(pixel* image, int w, int h, int final_w, int final_h);
void printAsciiImage(char* image, int col);
void setPointer(int x, int y);
void clearConsole();
