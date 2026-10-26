// Tjark Janke 260ADM037
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: claude, https://www.geeksforgeeks.org/c, https://introcs-python.cs.luc.edu/recursion/recursiondparser.html, https://coddy.tech/cheat-sheets/c, 
// I used my companies claude enterprise instance for this project and realised that I can't share the chat with people outside my organization.
// I have added the folder claude_screenshots were I documented the entire chat. If necessary I can also show the entire chat in person at uni.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#define USAGE "Usage: %s [-d DIR | --dir DIR] [-o OUTDIR | --output-dir OUTDIR] [input.txt]\n"

/* ---- Student data: change these ----------------------------------------- */
#define STUDENT_NAME     "tjark"
#define STUDENT_LASTNAME "janke"
#define STUDENT_ID       "260ADM037"

#define DIR_SUFFIX  "_" STUDENT_NAME "_" STUDENT_ID
#define FILE_SUFFIX "_" STUDENT_NAME "_" STUDENT_LASTNAME "_" STUDENT_ID ".txt"

/*Integer and float arithmetic
    Decision between integer and float arithmetic is made using an if statement in the apply function.
    If either number of the calculation is a float we convert both of them to float and return a float.
    Otherwise we run the calculation using integers and return an integer.
    To track if the current number is an integer we create a struct called Number that contains a double property for floats,
    a long long property for integers and an integer that is used as a boolean to indicate whether the number is an integer or not
    This "boolean" is what gets checked during every function call of apply. 

*/

/*Grammar precedence
    The Grammar has the correct precedence because the lower functions call the higher precidency functions, these consume all matching characters before returning their value.
    E.g 2 + 3 * 4:
    Expr calls term. Term reads 2 and + and therefore returns 2.
    Expr calls term for 3. Term reads 3 and *. Because * matches its operator it continues reading and consumes 4. It calculates 3*4 = 12 which it returns to Expr.
    Expr calculates 2+12=14 and returns.

    An important distinction is Power which calls itself immediately for the exponent instead of consuming from left to right. This is what makes it right-assosiative.
*/



typedef struct {
    const char *dir;
    const char *file;
    const char *outdir;
} Options;

typedef enum{
    TOK_NUM,
    TOK_OP,
    TOK_INVALID,
    TOK_END
} TokenType;

typedef enum{
    OP_ADD = '+',
    OP_SUB = '-',
    OP_MUL = '*',
    OP_DIV = '/',
    OP_POW = '^', /* Is used as the power character inside the program. This does not affect tokenization/parsing*/
    OP_L = '(',
    OP_R = ')'
} Operator;


//Fields floatValue, intValue and isInteger only valid with TokenType TOK_NUM
//The field op is only valid with TokenType TOK_OP
//TOK_END and TOK_INVALID do not use other fields
typedef struct{
    TokenType type;
    double floatValue;
    long long intValue;
    int isInteger;
    Operator op;
    long long position;
} Token;

typedef struct {
    Token *tokens;
    size_t pos;
    long long error;
} Parser;

typedef struct {
    long long position;
    double floatValue;
    long long intValue;
    int isInteger;
} Number;

Number expr(Parser *p);
Number term(Parser *p);
Number power(Parser *p);
Number primary(Parser *p);


double convertToDouble(Number num){
    return num.isInteger ? (double)num.intValue : num.floatValue;
}


/*originally implemented by me but modified by Claude during the Prompt:
AI(Claude): "Analyse the file and implement this error handling: [5. Rules/Positions part]
Also implement the comment functionality for the files as described here, remember to count the characters in the comment line as well 
as its important for the errors: [5. Rules/Lines and output]"
modification now includes the square and multiply algorithm
Function expects an exponent >= 0 */
long long integerPower(long long base, long long exp){
    long long result = 1;
    while (exp > 0) {
        if (exp & 1) result *= base;
        exp >>= 1;
        if (exp > 0) base *= base;
    }
    return result;
}

/* 
Remembers only the first error; later ones are ignored. 
Where errors are handled:
- invalid character: is flagged in the tokenizer, it stops and the expression until the invalid character is evaluated. If the syntax error is the first error, then that is output
- two operators in a row: is flagged by the primary function because it expects a number or ( after an operator
- missing ) is also reported in the primary after the ( has been evaluated
- extra ) is flagged in evaluate_line
- value errors like division by zero, negative integer exponent and inf/NaN are handled in the apply function
More details about the error handling can be found above each function
*/

static void set_error(Parser *p, long long position){
    if (p->error == 0) p->error = position;
}

//Adopted from https://introcs-python.cs.luc.edu/recursion/recursiondparser.html
int match(Parser *p, Operator op){
    Token t = p->tokens[p->pos];

    if(t.type == TOK_OP && t.op == op) {
        p->pos++;
        return 1;
    }
    return 0;
}

/* 
Handle the errors division by zero, negative integer exponent and infinite/NaN float
These errors are handled here because the results of calculations from before need to be present
e.g 2 / (5-5) needs to be handled but can't be during tokenizing as the tokenizer does not know that the result of 5-5 is 0
Same goes for the negative integer exponent
And for the inf/NaN result as we obviously have to calculate first to be able to check if the result is infinite/NaN

AI(Claude): "Analyse the file and implement this error handling: [5. Rules/Positions part]
Also implement the comment functionality for the files as described here, remember to count the characters in the comment line as well 
as its important for the errors: [5. Rules/Lines and output]"


The logic as existing in the function below was already completely present (apart from the error handling). Claude only moved it from the individual functions expr, term and power to one.
I chose to keep the change, as it makes the code a lot more readable and removes duplicate handling of the integer and float cases.
*/
static Number apply(Parser *p, Operator op, long long opPos, Number a, Number b){
    Number r;
    r.position = a.position;
    r.intValue = 0;
    r.floatValue = 0;
    r.isInteger = a.isInteger && b.isInteger;

    if (op == OP_DIV && (b.isInteger ? b.intValue == 0 : b.floatValue == 0.0)) {
        set_error(p, b.position);
        return r;
    }
    if (op == OP_POW && r.isInteger && b.intValue < 0) {
        set_error(p, b.position);
        return r;
    }

    if (r.isInteger) {
        switch (op) {
            case OP_ADD: r.intValue = a.intValue + b.intValue; break;
            case OP_SUB: r.intValue = a.intValue - b.intValue; break;
            case OP_MUL: r.intValue = a.intValue * b.intValue; break;
            case OP_DIV: r.intValue = a.intValue / b.intValue; break;
            default:     r.intValue = integerPower(a.intValue, b.intValue); break;
        }
    } else {
        double x = convertToDouble(a), y = convertToDouble(b);
        switch (op) {
            case OP_ADD: r.floatValue = x + y; break;
            case OP_SUB: r.floatValue = x - y; break;
            case OP_MUL: r.floatValue = x * y; break;
            case OP_DIV: r.floatValue = x / y; break;
            default:     r.floatValue = pow(x, y); break;
        }
        if (!isfinite(r.floatValue)) {
            set_error(p, opPos);
        }
    }
    return r;
}


/*Adopted from https://introcs-python.cs.luc.edu/recursion/recursiondparser.html
left-associative via loop
Written by me, slightly modified by AI(claude) during the implementation of the apply function for error handling.
*/
Number expr(Parser *p){
    Number total = term(p);
    while(!p->error && (match(p, OP_ADD) || match(p, OP_SUB)))
    {
        Token op = p->tokens[p->pos - 1];
        Number right = term(p);
        if (p->error) break;
        total = apply(p, op.op, op.position, total, right);
    }
    return total;
}

/*Adopted from https://introcs-python.cs.luc.edu/recursion/recursiondparser.html
left-associative via loop
Written by me, slightly modified by AI(claude) during the implementation of the apply function for error handling.
*/
Number term(Parser *p){
    Number total = power(p);
    while(!p->error && (match(p, OP_MUL) || match(p, OP_DIV)))
    {
        Token op = p->tokens[p->pos - 1];
        Number right = power(p);
        if (p->error) break;
        total = apply(p, op.op, op.position, total, right);
    }
    return total;
}

/*Adopted from https://introcs-python.cs.luc.edu/recursion/recursiondparser.html
right-associative via recursion
Written by me, slightly modified by AI(claude) during the implementation of the apply function for error handling.
Power is right associative because the exponents need to be evaluated first.
E.g 2**3**2 -> 2**9 != 8**2
The right-association is handled by calling power itself for the exponent.
*/
Number power(Parser *p){
    Number base = primary(p);

    if(!p->error && match(p, OP_POW)) {
        Token op = p->tokens[p->pos - 1];
        Number exponent = power(p);
        if (!p->error) {
            base = apply(p, OP_POW, op.position, base, exponent);
        }
    }
    return base;
}


/*
Adopted from https://introcs-python.cs.luc.edu/recursion/recursiondparser.html
Handles missing right parenthesis. 
The error is handled here because we can only check for missing right parenthesis after passing the left parenthesis and the content inside it.
Also handles unexpected valid operators at an unexpected position. The error is handled here because it is "catch all" of the recursive functions.
So if nothing until here could handle the operator then it must not be valid. This also includes completely invalid characters which are denoted using the TOK_INVALID which no function can handle and therefore lands here
The reason we handle invalid tokens here and not during the tokenizer is, that in theory another error could occur first
Written by me, slightly modified by AI(claude) during the implementation of the apply function for error handling.
*/
Number primary(Parser *p){
    Token t = p->tokens[p->pos];
    Number num;

    num.position = t.position;
    num.intValue = 0;
    num.floatValue = 0;
    num.isInteger = 1;

    if(t.type == TOK_NUM) {
        p->pos++;
        num.isInteger = t.isInteger;
        num.intValue = t.intValue;
        num.floatValue = t.floatValue;
        return num;
    }

    if (match(p, OP_L)) {
        num = expr(p);
        if (p->error) return num;
        if (!match(p, OP_R)) {
            set_error(p, p->tokens[p->pos].position);
            return num;
        }
        num.position = t.position;
        return num;
    }

    set_error(p, t.position);
    return num;
}

//Offset specifies the position of the first character in the row in relation to the whole file
//Stops parsing after hitting an invalid token. Expression until the invalid token will be evaluated in case another error comes before it.

Token *tokenize(const char *text, size_t len, long long offset) {
    Token *tokens = malloc((len + 2) * sizeof *tokens);
    size_t count = 0, i = 0;

    if (tokens == NULL) return NULL;

    while (i < len) {
        char c = text[i];
        Token *t = &tokens[count];

        if (isspace((unsigned char)c)) {
            i++;
            continue;
        }
        t->position = offset + (long long)i + 1;

        if (c != '\0' && strchr("+-*/()", c) != NULL) {
            t->type = TOK_OP;
            if (c == '*' && i + 1 < len && text[i + 1] == '*') {
                t->op = OP_POW;
                i += 2;
            } else {
                t->op = (Operator)c;
                i++;
            }
        }
        else if (isdigit((unsigned char)c)) {
            size_t j = i;
            int isFloat = 0;

            while (j < len && isdigit((unsigned char)text[j])) j++;
            if (j + 1 < len && text[j] == '.' && isdigit((unsigned char)text[j +1])) {
                isFloat = 1;
                j++;
                while (j < len && isdigit((unsigned char)text[j])) j++;
            }

            t->type = TOK_NUM;
            t->isInteger = !isFloat;
            t->intValue = isFloat ? 0 : strtoll(&text[i], NULL, 10);
            t->floatValue = isFloat ? strtod(&text[i], NULL) : 0;
            i = j;
        }
        else {
            t->type = TOK_INVALID; 
            count++;
            break;
        }
        count++;
    }

    tokens[count].type = TOK_END;
    tokens[count].position = offset + (long long)len + 1;
    return tokens;
}



/*
AI(claude) prompt: "Add functionality to the input and output files that supports these requirements:[4. The program section from the requirements]"
*/
static int ends_with_txt(const char *s, size_t n)
{
    return n > 4 && s[n - 4] == '.' &&
           s[n - 3] == 't' &&
           s[n - 2] == 'x' &&
           s[n - 1] == 't';
}

/*
AI(claude) prompt: "Add functionality to the input and output files that supports these requirements:[4. The program section from the requirements]"
*/
static char *build(const char *dir, const char *name, int len, const char *suffix)
{
    size_t dlen = strlen(dir);
    const char *sep = (dlen == 0 || dir[dlen - 1] == '/') ? "" : "/";
    char *r = malloc(dlen + 1 + (size_t)len + strlen(suffix) + 1);

    if (r != NULL) sprintf(r, "%s%s%.*s%s", dir, sep, len, name, suffix);
    return r;
}

/*
* AI(Claude) prompt: "Please refactor the list-txt_files function and simplify it"
*/
static int cmp_str(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
}

/*
 * AI(Claude): prompt "Write me a function in the C programming language that takes a path as input and returns all .txt filesnames.
 * You are only allowed to use the libraries stdio.stdlib,string ctype, math,errno,dirent, sys/stat"
*/
void free_txt_files(char **list)
{
    size_t i;

    if (list == NULL) {
        return;
    }
    for (i = 0; list[i] != NULL; i++) {
        free(list[i]);
    }
    free(list);
}


/*
 * Lists all regular files ending in ".txt" in the directory at `path`.
 *
 * AI(Claude): prompt "Write me a function in the C programming language that takes a path as input and returns all .txt filesnames.
 * You are only allowed to use the libraries stdio.stdlib,string ctype, math,errno,dirent, sys/stat"
 * 
 * Later modified via: AI(Claude) prompt: "Please refactor the list-txt_files function and simplify it"
 */
char **list_txt_files(const char *dir, size_t *count)
{
    size_t n = 0, cap = 16;
    char **list, **grown, *path;
    struct dirent *e;
    struct stat st;
    DIR *d;
    int err;

    if ((list = malloc(cap * sizeof *list)) == NULL) return NULL;
    if ((d = opendir(dir)) == NULL) {
        free(list);
        return NULL;
    }
    list[0] = NULL;

    while ((errno = 0, e = readdir(d)) != NULL) {
        size_t len = strlen(e->d_name);

        if (!ends_with_txt(e->d_name, len)) continue;
        if ((path = build(dir, e->d_name, (int)len, "")) == NULL) goto fail;
        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
            free(path);   /* e.g. a folder named x.txt */
            continue;
        }
        if (n + 1 == cap) {
            if ((grown = realloc(list, (cap *= 2) * sizeof *list)) == NULL) {
                free(path);
                goto fail;
            }
            list = grown;
        }
        list[n++] = path;
        list[n] = NULL;
    }
    if (errno != 0) goto fail;

    closedir(d);
    qsort(list, n, sizeof *list, cmp_str);
    *count = n;
    return list;

fail:
    err = errno ? errno : ENOMEM;
    closedir(d);
    free_txt_files(list);
    errno = err;
    return NULL;
}

/*AI(claude) prompt: "Write a commandline argument parser for c. It should be able to handle the following arguments ProgramName [-d DIR | --dir DIR]
 [-o OUTDIR | --output-dir OUTDIR] [input.txt]
 It can either take the input DIR or the single file, not both.
 In addition, also write af unctin that can create the OUTDIR if it doesnt exist."
*/
static int is_opt(const char *arg, const char *s, const char *l)
{
    size_t n = strlen(l);
    return !strcmp(arg, s) || (!strncmp(arg, l, n) && (arg[n] == '\0' || arg[n] == '='));
}


/*AI(claude) prompt: "Write a commandline argument parser for c. It should be able to handle the following arguments ProgramName [-d DIR | --dir DIR]
 [-o OUTDIR | --output-dir OUTDIR] [input.txt]
 It can either take the input DIR or the single file, not both.
 In addition, also write af unctin that can create the OUTDIR if it doesnt exist."
*/
static int set_opt(const char **dest, int argc, char **argv, int *i, const char *name)
{
    const char *eq = strchr(argv[*i], '=');
    const char *val = (argv[*i][1] == '-' && eq) ? eq + 1
                    : (*i + 1 < argc ? argv[++*i] : NULL);

    if (*dest)
        return fprintf(stderr, "Error: %s given more than once\n", name), -1;
    if (!val || !*val || (val[0] == '-' && val[1]))
        return fprintf(stderr, "Error: %s requires a value\n", name), -1;
    *dest = val;
    return 0;
}


/*AI(claude) prompt: "Write a commandline argument parser for c. It should be able to handle the following arguments ProgramName [-d DIR | --dir DIR]
 [-o OUTDIR | --output-dir OUTDIR] [input.txt]
 It can either take the input DIR or the single file, not both.
 In addition, also write af unctin that can create the OUTDIR if it doesnt exist."
*/
int parse_args(int argc, char **argv, Options *o)
{
    int i, end = 0;

    o->dir = o->file = o->outdir = NULL;
    for (i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!end && !strcmp(a, "--"))
            end = 1;
        else if (!end && is_opt(a, "-d", "--dir")) {
            if (set_opt(&o->dir, argc, argv, &i, "--dir")) return -1;
        } else if (!end && is_opt(a, "-o", "--output-dir")) {
            if (set_opt(&o->outdir, argc, argv, &i, "--output-dir")) return -1;
        } else if (!end && a[0] == '-' && a[1])
            return fprintf(stderr, "Error: unknown option '%s'\n", a), -1;
        else if (o->file)
            return fprintf(stderr, "Error: only one input file allowed\n"), -1;
        else
            o->file = a;
    }
    if (!o->dir == !o->file)
        return fprintf(stderr, "Error: give either -d DIR or an input file\n"), -1;
    return 0;
}

/*AI(claude) prompt: "Write a commandline argument parser for c. It should be able to handle the following arguments ProgramName [-d DIR | --dir DIR]
 [-o OUTDIR | --output-dir OUTDIR] [input.txt]
 It can either take the input DIR or the single file, not both.
 In addition, also write af unctin that can create the OUTDIR if it doesnt exist."

 Modified using: 
 AI(claude) prompt: "The code can assume that the parent directory already exists. What does that change?"
*/
int create_directory(const char *path)
{
    struct stat st;

    if (mkdir(path, 0755) == 0) return 0;
    if (errno != EEXIST || stat(path, &st) != 0) return -1;
    if (!S_ISDIR(st.st_mode)) {errno = ENOTDIR; return -1;}
    return 0;
}

/*
AI(claude) prompt: "Add functionality to the input and output files that supports these requirements:[4. The program section from the requirements]"
*/
static const char *input_base(const char *path, int strip_txt, int *len)
{
    size_t end = strlen(path), start;

    while (end > 1 && path[end - 1] == '/') end--;
    for (start = end; start > 0 && path[start - 1] != '/'; start--);
    if (strip_txt && ends_with_txt(path + start, end - start)) end -= 4;
    *len = (int)(end - start);
    return path + start;
}





/* 
AI(claude) prompt: "This is my current file. As of right now it reads every file entirely and then processes it. 
To make it possible to read larger files I instead want to process each line on its own. Each line is one calculation. 
However due to the way errors are handled I need to keep the same pos and increase it accross all rows."
*/
static long long evaluate_line(const char *line, size_t len, long long offset, Number *result){
    Token *tokens = tokenize(line, len, offset);
    Parser parser;

    if (tokens == NULL) return -1;
    parser.tokens = tokens;
    parser.pos = 0;
    parser.error = 0; /* 0 indicates no error*/

    *result = expr(&parser);

    if (parser.error == 0 && tokens[parser.pos].type != TOK_END) {
        parser.error = tokens[parser.pos].position;
    }
    free(tokens);
    return parser.error;
}

/*
Returns either -1 for read error, 1 for success and 0 for end of file

AI(claude) prompt: "This is my current file. As of right now it reads every file entirely and then processes it. 
To make it possible to read larger files I instead want to process each line on its own. Each line is one calculation. 
However due to the way errors are handled I need to keep the same pos and increase it accross all rows."
*/
static int read_line(FILE *fp, char **line, size_t *len)
{
    size_t n = 0, cap = 128;
    char *buf = malloc(cap), *grown;
    int c;

    if (buf == NULL) return -1;
    while ((c = fgetc(fp)) != EOF && c != '\n') {
        if (n + 1 >= cap) {
            if ((grown = realloc(buf, cap *= 2)) == NULL) {
                free(buf);
                return -1;
            }
            buf = grown;
        }
        buf[n++] = (char)c;
    }
    if (c == EOF && n == 0) {
        free(buf);
        return 0;
    }
    buf[n] = '\0';
    *line = buf;
    *len = n;
    return 1;
}

/*
AI(Claude): "Analyse the file and implement this error handling: [5. Rules/Positions part]
Also implement the comment functionality for the files as described here, remember to count the characters in the comment line as well 
as its important for the errors: [5. Rules/Lines and output]"

AI(claude) prompt: "Add functionality to the input and output files that supports these requirements:[4. The program section from the requirements]"
*/
static int skip_line(const char *line, size_t len)
{
    size_t i = 0;

    while (i < len && isspace((unsigned char)line[i])) i++;
    return i == len || line[i] == '#';
}


void writeResult(FILE *fp, Number result){
    if(result.isInteger)
    {
        fprintf(fp, "%lld\n", result.intValue);
    }
    else
    {
        fprintf(fp, "%.15g\n", result.floatValue);
    }
}

/*
    Large file handling:
    The entire file is never loaded as a whole. Instead we read and evaluate each line individually, growing the list for the line dynamically to ensure we use the least possible amount of memory.


    
    We track the position of each token by using an offset in the process_file function that gets increased by
    the length of each line after the line has been tokenized. In the tokenize function the offset is used as the baseline.
    The current position in the loop of tokenize is added onto offset (and one is added in addition because the error is supposed to be 1 indexed)
    We use the offset to make it possible to keep one index across multiple rows.

    Error gets written to file if the returned value is not null.

    Each line is evaluated one at a time and one result is written.
    The result can either be the successful result of an expression or an error
    AI(claude) prompt: "This is my current file. As of right now it reads every file entirely and then processes it. 
    To make it possible to read larger files I instead want to process each line on its own. Each line is one calculation. 
    However due to the way errors are handled I need to keep the same pos and increase it accross all rows."
*/
int process_file(const char *in_path, const char *outdir){
    int len_base, status;
    const char *base = input_base(in_path, 1, &len_base);
    char *out_path = build(outdir, base, len_base, FILE_SUFFIX);
    char *line;
    size_t len;
    long long offset = 0, error = 0;
    FILE *in, *out;

    if (out_path == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        return -1;
    }

    if ((in = fopen(in_path, "rb")) == NULL) {
        fprintf(stderr, "Error: cannot read '%s': %s\n", in_path, strerror(errno));
        free(out_path);
        return -1;
    }
    if ((out = fopen(out_path, "w")) == NULL) {
        fprintf(stderr, "Error: cannot write '%s': %s\n", out_path, strerror(errno));
        fclose(in);
        free(out_path);
        return -1;
    }

    while ((status = read_line(in, &line, &len)) == 1) {
        if (!skip_line(line, len)) {
            Number result;
            error = evaluate_line(line, len, offset, &result);
            if (error == 0) writeResult(out, result);
        }
        offset += (long long)len + 1;
        free(line);
        if (error != 0) break;
    }
    fclose(in);

    if (status < 0 || error < 0) {
        fprintf(stderr, "Error: out of memory while reading '%s'\n", in_path);
        fclose(out);
        free(out_path);
        return -1;
    }

    if (error > 0) {
        if ((out = freopen(out_path, "w", out)) == NULL) {
            fprintf(stderr, "Error: cannot write '%s': %s\n", out_path, strerror(errno));
            free(out_path);
            return -1;
        }
        fprintf(out, "ERROR:%lld\n", error);
    }

    fclose(out);
    free(out_path);
    return 0;
}


/*Base main function written by me but modified multiple times by claude changes:

AI(claude) prompt: "This is my current file. As of right now it reads every file entirely and then processes it. 
To make it possible to read larger files I instead want to process each line on its own. Each line is one calculation. 
However due to the way errors are handled I need to keep the same pos and increase it accross all rows."

AI(claude) prompt: "Write a commandline argument parser for c. It should be able to handle the following arguments ProgramName [-d DIR | --dir DIR]
 [-o OUTDIR | --output-dir OUTDIR] [input.txt]
 It can either take the input DIR or the single file, not both.
 In addition, also write af unctin that can create the OUTDIR if it doesnt exist."
*/

int main(int argc, char *argv[])
{
    Options o;
    struct stat st;
    const char *in, *outdir, *base;
    char *default_dir = NULL;
    int len; 
    int status = EXIT_FAILURE;

    if (parse_args(argc, argv, &o)) {
        fprintf(stderr, USAGE, argv[0]);
        return EXIT_FAILURE;
    }

    in = o.dir ? o.dir : o.file;
    if (stat(in, &st) || (o.dir ? !S_ISDIR(st.st_mode) : !S_ISREG(st.st_mode))) {
        fprintf(stderr, "Error: '%s' is not a valid %s\n", in, o.dir ? "directory" : "file");
        return EXIT_FAILURE;
    }

    if (o.outdir) {
        outdir = o.outdir;
    } else {
        base = input_base(in, o.file != NULL, &len);
        if ((default_dir = build("", base, len, DIR_SUFFIX)) == NULL) {
            fprintf(stderr, "Error: out of memory\n");
            return EXIT_FAILURE;
        }
        outdir = default_dir;
    }

    if (create_directory(outdir)) {
        fprintf(stderr, "Error: cannot create '%s': %s\n", outdir, strerror(errno));
        goto done;
    }

    if (o.file) {
        status = process_file(in, outdir) ? EXIT_FAILURE : EXIT_SUCCESS;
    } else {
        size_t n, i;
        int failed = 0;
        char **files = list_txt_files(in, &n);

        if (files == NULL) {
            fprintf(stderr, "Error reading '%s': %s\n", in, strerror(errno));
            goto done;
        }
        if (n == 0) printf("No .txt files found in '%s'\n", in);
        for (i = 0; i < n; i++) {
            if (process_file(files[i],outdir) != 0) failed = 1;
        }
        free_txt_files(files);
        status = failed ? EXIT_FAILURE : EXIT_SUCCESS;
    }

done:
    free(default_dir);
    return status;
}
