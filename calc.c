#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum{
    TOK_NUM,
    TOK_OP,
    TOK_END
} TokenType;

typedef enum{
    OP_ADD = '+',
    OP_SUB = '-',
    OP_MUL = '*',
    OP_DIV = '/',
    OP_POW = '^',
    OP_L = '(',
    OP_R = ')'
} Operator;

typedef struct{
    TokenType type;
    double value;
    Operator op;
} Token;


//Read file function on the basis of: https://www.fundza.com/c4serious/fileIO_reading_all/index.html
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
        fclose(infile);
        exit(1);
    }
    
    int n = fread(buffer, sizeof(char), numbytes, infile);
    buffer[n] = '\0';
    fclose(infile);
    return buffer;
}


void print_tokens(const Token *tokens){
    for(size_t i = 0; tokens[i].type != TOK_END; i++)
    {
        switch (tokens[i].type) {
            case TOK_NUM: printf("NUM(%g) ", tokens[i].value); break;
            case TOK_OP: printf("OP(%c) ", (char)tokens[i].op); break;
            case TOK_END: break;
        }
    }
    printf("END\n");
}


Token *tokenize(char *text) {
    int i = 0;

    Token *tokens = malloc((strlen(text) + 1) * sizeof(Token));
    if (tokens == NULL) {
        perror("malloc");
        exit(1);
    }

    size_t count = 0;

    while (text[i] != '\0') {
        char currentChar = text[i];

        
        if (isspace((unsigned char)currentChar)) {
            i++;
            continue;
        }

        if (strchr("+-*/()", currentChar) != NULL ||
            currentChar == '*') {

            tokens[count].type = TOK_OP;

            if (currentChar == '*' && text[i + 1] == '*') {
                tokens[count].op = OP_POW;
                i += 2;
            } else {
                tokens[count].op = (Operator)currentChar;
                i++;
            }

            count++;
        }

        else if (isdigit((unsigned char)currentChar)) {

            char *end;
            double value = strtod(&text[i], &end);

            if (end == &text[i]) {
                printf("Couldnt read number :(");
                free(tokens);
                exit(1);
            }

            tokens[count].type = TOK_NUM;
            tokens[count].value = value;
            count++;

            i = (int)(end - text);
        }
        else
        {
            printf("Invalid token");
            free(tokens);
            exit(1);
        }
    }

    tokens[count].type = TOK_END;
    return tokens;
    free(tokens);
}



int main(void) {
    char * text = read_file("/home/tjark/Graded-Lab-1/testfile.txt");
    printf("%s\n", text);
    Token * tokens = tokenize(text);
    print_tokens(tokens);
    
}