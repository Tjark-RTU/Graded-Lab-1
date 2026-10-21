#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>

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

typedef struct {
    Token *tokens;
    size_t pos;
} Parser;

double expr(Parser *p);
double term(Parser *p);
double unary(Parser *p);
double power(Parser *p);
double primary(Parser *p);

//Adopted from: https://introcs-python.cs.luc.edu/recursion/recursiondparser.html
bool match(Parser *p, Operator op)
{
    Token t = p->tokens[p->pos];

    if(t.type == TOK_OP && t.op == op) {
        p->pos++;
        return true;
    }
    return false;
}

//Adopted from: https://introcs-python.cs.luc.edu/recursion/recursiondparser.html
double expr(Parser *p)
{
    double total = term(p);
    while(match(p, OP_ADD) || match(p, OP_SUB))
    {
        Operator op = p->tokens[p->pos -1].op;
        double right = term(p);

        if(op == OP_ADD)
        {
            total += right;
        }
        else
        {
            total -= right;
        }
    }
    return total;
}

//Adopted from: https://introcs-python.cs.luc.edu/recursion/recursiondparser.html
double term(Parser *p)
{
    double total = unary(p);
    while(match(p, OP_MUL) || match(p, OP_DIV))
    {
        Operator op = p->tokens[p->pos -1].op;
        double right = unary(p);

        if(op == OP_MUL)
        {
            total *= right;
        }
        else
        {
            total /= right;
        }
    }
    return total;

}

//Adopted from: https://introcs-python.cs.luc.edu/recursion/recursiondparser.html
double unary(Parser *p){
    if (match(p, OP_SUB)) {
        return -unary(p);
    }
    if(match(p, OP_ADD)){
        return unary(p);
    }
    return power(p);
}

//Adopted from: https://introcs-python.cs.luc.edu/recursion/recursiondparser.html
double power(Parser *p){
    double base = primary(p);

    if(match(p, OP_POW)) {
        double exponent = power(p);
        return pow(base, exponent);
    }
    return base;
}

//Adopted from: https://introcs-python.cs.luc.edu/recursion/recursiondparser.html
double primary(Parser *p){
    Token t = p->tokens[p->pos];
    
    if(t.type == TOK_NUM) {
        p->pos++;
        return t.value;
    }

    if (match(p, OP_L)) {
        double value = expr(p);
        return value;
    }
    exit(1);
}

//Adopted from: https://introcs-python.cs.luc.edu/recursion/recursiondparser.html
double parse(Parser *p){
    double result = expr(p);

    return result;
}


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
    Parser parser;
    parser.tokens = tokens;
    parser.pos = 0;
    double result = parse(&parser);
    printf("Result: %f\n", result);
    
    
}