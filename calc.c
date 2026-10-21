#include <stdio.h>
#include <stdlib.h>



//Read file function on the basis off: https://www.fundza.com/c4serious/fileIO_reading_all/index.html
char * read_file(const char*path){
     	
    FILE    *infile;
    char    *buffer;
    long    numbytes;
    
    infile = fopen(path, "r");
    

    if(infile == NULL)
    {
        exit(1);
    }
    

    fseek(infile, 0L, SEEK_END);
    numbytes = ftell(infile);
    
    fseek(infile, 0L, SEEK_SET);	
    
    buffer = (char*)calloc(numbytes + 1, sizeof(char));	
    
    if(buffer == NULL)
    {
        exit(1);
    }
    
    int n = fread(buffer, sizeof(char), numbytes, infile);
    buffer[n] = '\0';
    fclose(infile);
    return buffer;
}






int main(void) {
    char * text = read_file("/home/tjark/Graded-Lab-1/testfile.txt");
    printf("%s\n", text);
}