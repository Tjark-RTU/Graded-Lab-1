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
    double floatValue;
    long intValue;
    bool isInteger;
    Operator op;
    int position; 
} Token;

typedef struct {
    Token *tokens;
    size_t pos;
} Parser;

typedef struct {
    int position;
    double floatValue;
    long intValue;
    bool isInteger;
} Number;

Number expr(Parser *p);
Number term(Parser *p);
Number power(Parser *p);
Number primary(Parser *p);


double convertToDouble(Number num){
    if(num.isInteger){
        return (float)num.intValue;
    }
    return num.floatValue;
}

int minPosition(Number num1, Number num2){
    if(num1.position < num2.position){
        return num1.position;
    }
    return num2.position;
}

long integerPower(long num1, long num2){
    long result = num1;
    for(int i = 1; i < num2; i++){
        result = result * num1;
    }
    return result;
}


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
Number expr(Parser *p)
{
    Number total = term(p);
    while(match(p, OP_ADD) || match(p, OP_SUB))
    {
        Operator op = p->tokens[p->pos -1].op;
        Number right = term(p);

        Number num;
        num.position = minPosition(total, right);
        if(op == OP_ADD)
        {
            if(total.isInteger == false || right.isInteger == false){
                num.floatValue = convertToDouble(total) + convertToDouble(right);
                num.intValue = 0;
                num.isInteger = false;
            }
            else{
                num.intValue = total.intValue + right.intValue;
                num.floatValue = 0;
                num.isInteger = true;
            }
        }
        else
        {
            if(total.isInteger == false || right.isInteger == false){
                num.floatValue = convertToDouble(total) - convertToDouble(right);
                num.intValue = 0;
                num.isInteger = false;
            }
            else{
                num.intValue = total.intValue - right.intValue;
                num.floatValue = 0;
                num.isInteger = true;
            }
        }
        total = num;
    }
    return total;
}



//Adopted from: https://introcs-python.cs.luc.edu/recursion/recursiondparser.html
Number term(Parser *p)
{
    Number total = power(p);
    while(match(p, OP_MUL) || match(p, OP_DIV))
    {
        Operator op = p->tokens[p->pos -1].op;
        Number right = power(p);


        Number num;
        num.position = minPosition(total, right);
        if(op == OP_MUL)
        {
            if(total.isInteger == false || right.isInteger == false){
                
                num.floatValue = convertToDouble(total) * convertToDouble(right);
                num.intValue = 0;
                num.isInteger = false;
            }
            else{
                num.intValue = total.intValue * right.intValue;
                num.floatValue = 0;
                num.isInteger = true;
            }
        }
        else
        {
            if(total.isInteger == false || right.isInteger == false){
                num.floatValue = convertToDouble(total) / convertToDouble(right);
                num.intValue = 0;
                num.isInteger = false;
            }
            else{
                num.intValue = total.intValue / right.intValue;
                num.floatValue = 0;
                num.isInteger = true;
            }
        }
        total = num;
    }
    return total;

}


//Adopted from: https://introcs-python.cs.luc.edu/recursion/recursiondparser.html
Number power(Parser *p){
    Number base = primary(p);

    if(match(p, OP_POW)) {
        Number exponent = power(p);
        Number num;
        num.position = minPosition(base, exponent);

        if(base.isInteger == false || exponent.isInteger == false){
            num.floatValue = pow(convertToDouble(base), convertToDouble(exponent));
            num.intValue = 0;
            num.isInteger = false;
            return num;
        }
        else{
            num.intValue = integerPower(base.intValue,exponent.intValue);
            num.floatValue = 0;
            num.isInteger = true;
            return num;
        }
    }
    return base;
}

//Adopted from: https://introcs-python.cs.luc.edu/recursion/recursiondparser.html
Number primary(Parser *p){
    Token t = p->tokens[p->pos];
    
    if(t.type == TOK_NUM) {
        p->pos++;
        Number num;
        num.position = t.position;
        if(t.isInteger){
            printf("ValueInt: %ld\n",t.intValue);
            num.intValue = t.intValue;
            num.floatValue = 0;
            num.isInteger = true;
            return num;
        }
        else{
            printf("ValueFloat: %f\n",t.floatValue);
            num.floatValue = t.floatValue;
            num.intValue = 0;
            num.isInteger = false;
            return num;
        }
    }

    if (match(p, OP_L)) {
        Number value = expr(p);
        if (!match(p, OP_R)) {
            printf("Error: expected ')'\n");
            exit(1);
        }
        return value;
    }
    exit(1);
}

//Adopted from: https://introcs-python.cs.luc.edu/recursion/recursiondparser.html
Number parse(Parser *p){
    Number result = expr(p);

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
            case TOK_NUM: printf("NUM(%g is float:%d)", (tokens[i].intValue + tokens[i].floatValue),tokens[i].isInteger); break;
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
            tokens[count].position = count + 1;
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

            char *endFloat;
            double valueFloat = strtod(&text[i], &endFloat);
            char *endInt;
            long valueInt = strtol(&text[i], &endInt,10); 

            
            

            if (endFloat == &text[i]) {
                printf("Couldnt read number :(");
                free(tokens);
                exit(1);
            }

            tokens[count].type = TOK_NUM;
            tokens[count].position = count + 1;
            

            if(endFloat == endInt){
                printf("ValueInt: %ld\n",valueInt);
                tokens[count].intValue = valueInt;
                tokens[count].floatValue = 0;
                tokens[count].isInteger = true;
                i = (int)(endInt - text);

            }
            else{
                printf("ValueFloat: %f\n",valueFloat);
                tokens[count].floatValue = valueFloat;
                tokens[count].intValue = 0;
                tokens[count].isInteger = false;
                i = (int)(endFloat - text);
            }
            count++;



            
        }
        else
        {
            printf("Invalid token position: %d", (int)(count +1));
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
    Number result = parse(&parser);
    if(result.isInteger){
        printf("Result: %ld\n", result.intValue);
    }
    else{
        printf("Result: %f\n", result.floatValue);
    }
    printf("IntegerPower test: %ld\n", integerPower(2,3));
    
}