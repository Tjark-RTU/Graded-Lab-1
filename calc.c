#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

//ToDo: Remove booleans because apparently we are not allowed to use them?????
#define USAGE "Usage: %s [-d DIR | --dir DIR] [-o OUTDIR | --output-dir OUTDIR] [input.txt]\n"

typedef struct {
    const char *dir;     /* -d / --dir         */
    const char *file;    /* positional input   */
    const char *outdir;  /* -o / --output-dir  */
} Options;

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

char * outputFolderPath(int argc, char *argv[]){
    for(int i = 0; i < argc; i++)
    {
        if(strcmp(argv[i],"-o") == 0)
        {
            return argv[i +1];
        }
    }
    return NULL;
}

char * inputFolderPath(int argc, char *argv[]){
    for(int i = 0; i < argc; i++)
    {
        if(strcmp(argv[i],"-d") == 0)
        {
            return argv[i +1];
        }
    }
    return NULL;
}

char * inputFilePath(int argc, char *argv[]){
    for(int i = 0; i< argc; i++)
    {
        if(strcmp(argv[i], "-o") && i + 3 <= argc){
            printf("%s\n", argv[i + 3]);
            return argv[i + 2];
        }
    }
    return NULL;
}


//ToDo add AI Documentation
//Start written with claude
/* Portable strdup replacement (strdup is POSIX, not standard C). */
static char *dup_string(const char *s)
{
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (copy != NULL) {
        memcpy(copy, s, len);
    }
    return copy;
}

/* Returns 1 if name ends in ".txt" (case-insensitive) and has a base name. */
static int has_txt_extension(const char *name)
{
    size_t len = strlen(name);
    const char *ext;

    if (len <= 4) {
        return 0;
    }
    ext = name + len - 4;
    return ext[0] == '.' &&
           tolower((unsigned char)ext[1]) == 't' &&
           tolower((unsigned char)ext[2]) == 'x' &&
           tolower((unsigned char)ext[3]) == 't';
}

/*
 * Lists all regular files ending in ".txt" in the directory at `path`.
 *
 * Returns a NULL-terminated array of newly allocated filename strings and
 * stores the number of names in *out_count. Free the result with
 * free_txt_files(). On error, returns NULL and sets errno.
 * An empty directory returns a valid array containing only NULL.
 */
char **list_txt_files(const char *path, size_t *out_count)
{
    DIR *dir;
    struct dirent *entry;
    char **list;
    size_t count = 0;
    size_t capacity = 16;
    size_t path_len;
    int needs_sep;
    int saved_errno;

    if (path == NULL || out_count == NULL) {
        errno = EINVAL;
        return NULL;
    }
    *out_count = 0;

    dir = opendir(path);
    if (dir == NULL) {
        return NULL; /* errno set by opendir */
    }

    list = malloc(capacity * sizeof *list);
    if (list == NULL) {
        closedir(dir);
        errno = ENOMEM;
        return NULL;
    }

    path_len = strlen(path);
    needs_sep = (path_len > 0 && path[path_len - 1] != '/');

    for (;;) {
        const char *name;
        char *full_path;
        size_t full_len;
        struct stat st;

        errno = 0;
        entry = readdir(dir);
        if (entry == NULL) {
            if (errno != 0) {
                goto fail; /* real read error, not end of directory */
            }
            break;
        }

        name = entry->d_name;
        if (!has_txt_extension(name)) {
            continue;
        }

        /* Build "path/name" so stat() can confirm it's a regular file. */
        full_len = path_len + (size_t)needs_sep + strlen(name) + 1;
        full_path = malloc(full_len);
        if (full_path == NULL) {
            errno = ENOMEM;
            goto fail;
        }
        memcpy(full_path, path, path_len);
        if (needs_sep) {
            full_path[path_len] = '/';
        }
        strcpy(full_path + path_len + needs_sep, name);

        if (stat(full_path, &st) != 0 || !S_ISREG(st.st_mode)) {
            free(full_path);
            continue; /* skip directories named *.txt, broken links, etc. */
        }
        free(full_path);

        /* Grow the array, always leaving room for the NULL terminator. */
        if (count + 1 >= capacity) {
            char **grown = realloc(list, capacity * 2 * sizeof *list);
            if (grown == NULL) {
                errno = ENOMEM;
                goto fail;
            }
            list = grown;
            capacity *= 2;
        }

        list[count] = dup_string(name);
        if (list[count] == NULL) {
            errno = ENOMEM;
            goto fail;
        }
        count++;
    }

    list[count] = NULL;
    closedir(dir);
    *out_count = count;
    return list;

fail:
    saved_errno = errno;
    while (count > 0) {
        free(list[--count]);
    }
    free(list);
    closedir(dir);
    errno = saved_errno;
    return NULL;
}

/* True if arg is the short form, the long form, or "long=VALUE". */
static int is_opt(const char *arg, const char *s, const char *l)
{
    size_t n = strlen(l);
    return !strcmp(arg, s) || (!strncmp(arg, l, n) && (arg[n] == '\0' || arg[n] == '='));
}

/* Stores the option's value ("--opt=VAL" or the next argument) in *dest. */
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

/* Returns 0 on success, -1 on error (message already printed). */
int parse_args(int argc, char **argv, Options *o)
{
    int i, end = 0;  /* end = 1 after "--": everything is a filename */

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

/* Creates path and any missing parents (like mkdir -p).
   Returns 0 on success or if it already exists, -1 with errno set on error. */
int create_directory(const char *path)
{
    struct stat st;

    if (mkdir(path, 0755) == 0) return 0;
    if (errno != EEXIST || stat(path, &st) != 0) return -1;
    if (!S_ISDIR(st.st_mode)) {errno = ENOTDIR; return -1;}
    return 0;
}

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

//End written with claude


//Written by Claude to fix memory overflow issue
static size_t base_len(const char *name)
{
    size_t n = strlen(name);
    return has_txt_extension(name) ? n - 4 : n;
}

/* Returns a new string: the first len chars of a, followed by b.
   The caller must free() it. Returns NULL if out of memory. */
static char *concat(const char *a, size_t len, const char *b)
{
    size_t blen = strlen(b);
    char *r = malloc(len + blen + 1);

    if (r != NULL) {
        memcpy(r, a, len);
        memcpy(r + len, b, blen + 1);
    }
    return r;
}

/* "in" -> "in_hans_peter_2557", "notes.txt" -> "notes_hans_peter_2557".
   Caller frees. */
char *defaultOutputDir(const char *base)
{
    return concat(base, base_len(base), "_hans_peter_2557");
}

/* "notes.txt" -> "notes_hans_peter_2557.txt". Caller frees. */
char *outputFileName(const char *filename)
{
    return concat(filename, base_len(filename), "_hans_peter_2557.txt");
}
//End Written by claude

void calculate(char * text){
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


int main(int argc, char *argv[])
{
    Options o;
    struct stat st;
    const char *in, *kind, *outdir;
    char *default_dir = NULL;   /* only set if we allocated it */
    int status = EXIT_FAILURE;

    if (parse_args(argc, argv, &o)) {
        fprintf(stderr, USAGE, argv[0]);
        return EXIT_FAILURE;
    }

    in   = o.dir ? o.dir : o.file;
    kind = o.dir ? "directory" : "file";
    if (stat(in, &st) || (o.dir ? !S_ISDIR(st.st_mode) : !S_ISREG(st.st_mode))) {
        fprintf(stderr, "Error: '%s' is not a valid %s\n", in, kind);
        return EXIT_FAILURE;
    }

    if (o.outdir) {
        outdir = o.outdir;
    } else {
        default_dir = defaultOutputDir(in);
        if (default_dir == NULL) {
            fprintf(stderr, "Error: out of memory\n");
            return EXIT_FAILURE;
        }
        outdir = default_dir;
    }

    if (create_directory(outdir)) {
        fprintf(stderr, "Error: cannot create '%s': %s\n", outdir, strerror(errno));
        goto done;
    }
    printf("Input %s: %s\nOutput directory: %s\n", kind, in, outdir);

    if (o.dir) {
        size_t count, i;
        char **files = list_txt_files(in, &count);

        if (files == NULL) {
            fprintf(stderr, "Error reading '%s': %s\n", in, strerror(errno));
            goto done;
        }
        printf("Found %lu .txt file(s) in '%s':\n", (unsigned long)count, in);
        for (i = 0; i < count; i++) {
            printf("  %s\n", files[i]);
        }
        free_txt_files(files);
    }
    else{
        char * text = read_file(in);
        calculate(text);
    }

    status = EXIT_SUCCESS;

done:
    free(default_dir);   /* free(NULL) is a no-op */
    return status;
}