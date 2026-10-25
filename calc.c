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
#define STUDENT_NAME     "hans"
#define STUDENT_LASTNAME "peter"
#define STUDENT_ID       "2557"

#define DIR_SUFFIX  "_" STUDENT_NAME "_" STUDENT_ID
#define FILE_SUFFIX "_" STUDENT_NAME "_" STUDENT_LASTNAME "_" STUDENT_ID ".txt"



typedef struct {
    const char *dir;     /* -d / --dir         */
    const char *file;    /* positional input   */
    const char *outdir;  /* -o / --output-dir  */
} Options;

typedef enum{
    TOK_NUM,
    TOK_OP,
    TOK_INVALID,   /* a character that cannot start any token */
    TOK_END        /* end of line; positioned at the line's '\n' */
} TokenType;

typedef enum{
    OP_ADD = '+',
    OP_SUB = '-',
    OP_MUL = '*',
    OP_DIV = '/',
    OP_POW = '^',  /* written as ** in the input */
    OP_L = '(',
    OP_R = ')'
} Operator;

typedef struct{
    TokenType type;
    double floatValue;
    long long intValue;
    int isInteger;
    Operator op;
    long long position;   /* 1-based character position in the whole file */
} Token;

typedef struct {
    Token *tokens;
    size_t pos;
    long long error;      /* 0 = no error, otherwise position of the first error */
} Parser;

typedef struct {
    long long position;   /* where this operand starts in the file */
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

/* base ** exp for exp >= 0 (exponentiation by squaring). */
long long integerPower(long long base, long long exp){
    long long result = 1;
    while (exp > 0) {
        if (exp & 1) result *= base;
        exp >>= 1;
        if (exp > 0) base *= base;
    }
    return result;
}

/* Remembers only the first error; later ones are ignored. */
static void set_error(Parser *p, long long position){
    if (p->error == 0) p->error = position;
}

//Adopted from: https://urldefense.com/v3/__https://introcs-python.cs.luc.edu/recursion/recursiondparser.html__;!!LtPJ!2KQuQm9YLWM1UjipXEIacF02G1l4fHKy2wVawNrRQ-_XVRu7F9CLAw5BZkrg1eP_Hwj7dILu_TVq31dZkHPyUvmSoJk$ 
int match(Parser *p, Operator op)
{
    Token t = p->tokens[p->pos];

    if(t.type == TOK_OP && t.op == op) {
        p->pos++;
        return 1;
    }
    return 0;
}

/* Computes a <op> b. opPos is the operator's position (for inf/nan errors).
   On error, sets p->error and returns a dummy value. */
static Number apply(Parser *p, Operator op, long long opPos, Number a, Number b){
    Number r;
    r.position = a.position;
    r.intValue = 0;
    r.floatValue = 0;
    r.isInteger = a.isInteger && b.isInteger;

    if (op == OP_DIV && (b.isInteger ? b.intValue == 0 : b.floatValue == 0.0)) {
        set_error(p, b.position);              /* division by zero */
        return r;
    }
    if (op == OP_POW && r.isInteger && b.intValue < 0) {
        set_error(p, b.position);              /* negative integer exponent */
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
            set_error(p, opPos);               /* inf or nan */
        }
    }
    return r;
}

//Adopted from: https://urldefense.com/v3/__https://introcs-python.cs.luc.edu/recursion/recursiondparser.html__;!!LtPJ!2KQuQm9YLWM1UjipXEIacF02G1l4fHKy2wVawNrRQ-_XVRu7F9CLAw5BZkrg1eP_Hwj7dILu_TVq31dZkHPyUvmSoJk$ 
Number expr(Parser *p)
{
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

//Adopted from: https://urldefense.com/v3/__https://introcs-python.cs.luc.edu/recursion/recursiondparser.html__;!!LtPJ!2KQuQm9YLWM1UjipXEIacF02G1l4fHKy2wVawNrRQ-_XVRu7F9CLAw5BZkrg1eP_Hwj7dILu_TVq31dZkHPyUvmSoJk$ 
Number term(Parser *p)
{
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

//Adopted from: https://urldefense.com/v3/__https://introcs-python.cs.luc.edu/recursion/recursiondparser.html__;!!LtPJ!2KQuQm9YLWM1UjipXEIacF02G1l4fHKy2wVawNrRQ-_XVRu7F9CLAw5BZkrg1eP_Hwj7dILu_TVq31dZkHPyUvmSoJk$ 
Number power(Parser *p){
    Number base = primary(p);

    if(!p->error && match(p, OP_POW)) {
        Token op = p->tokens[p->pos - 1];
        Number exponent = power(p);   /* right-associative */
        if (!p->error) {
            base = apply(p, OP_POW, op.position, base, exponent);
        }
    }
    return base;
}

//Adopted from: https://urldefense.com/v3/__https://introcs-python.cs.luc.edu/recursion/recursiondparser.html__;!!LtPJ!2KQuQm9YLWM1UjipXEIacF02G1l4fHKy2wVawNrRQ-_XVRu7F9CLAw5BZkrg1eP_Hwj7dILu_TVq31dZkHPyUvmSoJk$ 
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
            /* missing ')': report whatever stands there, e.g. the '\n' */
            set_error(p, p->tokens[p->pos].position);
            return num;
        }
        num.position = t.position;   /* the operand starts at its '(' */
        return num;
    }

    /* Expected a number or '(' but found an operator, an invalid
       character, or the end of the line. */
    set_error(p, t.position);
    return num;
}


/* Splits one line (len characters) into tokens. offset is the number of
   characters in the file before this line, so a character at index i has
   position offset + i + 1.
   At the first invalid character a TOK_INVALID token is added and
   tokenizing stops; the parser reports it only if it gets that far, so an
   earlier parse error takes precedence. The final TOK_END sits at the
   line's '\n' (or file length + 1 on a last line without '\n').
   Returns NULL if out of memory. Caller frees. */
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
            /* digits [ . digits ] [ e [+-] digits ] */
            size_t j = i;
            int isFloat = 0;

            while (j < len && isdigit((unsigned char)text[j])) j++;
            if (j < len && text[j] == '.' && isdigit((unsigned char)text[j +1])) {
                isFloat = 1;
                j++;
                while (j < len && isdigit((unsigned char)text[j])) j++;
            }

            t->type = TOK_NUM;
            t->isInteger = !isFloat;
            t->intValue = isFloat ? 0 : strtol(&text[i], NULL, 10);
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



//ToDo add AI Documentation
//Start written with claude

static int ends_with_txt(const char *s, size_t n)
{
    return n > 4 && s[n - 4] == '.' &&
           tolower((unsigned char)s[n - 3]) == 't' &&
           tolower((unsigned char)s[n - 2]) == 'x' &&
           tolower((unsigned char)s[n - 1]) == 't';
}

/* Returns a new string "dir/" + first len chars of name + suffix.
   No "/" is added if dir is "" or already ends in '/'. Caller frees. */
static char *build(const char *dir, const char *name, int len, const char *suffix)
{
    size_t dlen = strlen(dir);
    const char *sep = (dlen == 0 || dir[dlen - 1] == '/') ? "" : "/";
    char *r = malloc(dlen + 1 + (size_t)len + strlen(suffix) + 1);

    if (r != NULL) sprintf(r, "%s%s%.*s%s", dir, sep, len, name, suffix);
    return r;
}

static int cmp_str(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
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

/*
 * Lists all regular files ending in ".txt" in the directory at `path`.
 *
 * Returns a NULL-terminated array of newly allocated filename strings and
 * stores the number of names in *out_count. Free the result with
 * free_txt_files(). On error, returns NULL and sets errno.
 * An empty directory returns a valid array containing only NULL.
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
    list[0] = NULL;   /* kept NULL-terminated so free_txt_files() works anytime */

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
    if (errno != 0) goto fail;   /* readdir failed, not just end of folder */

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

//End written with claude


//Written by Claude to fix memory overflow issue
static size_t base_len(const char *name)
{
    size_t n = strlen(name);
    return ends_with_txt(name, strlen(name)) ? n - 4 : n;
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

/* Last component of path ("labs/task1.txt" -> "task1.txt", "labs/" -> "labs").
   If strip_txt is set, a trailing ".txt" is excluded. Not NUL-terminated:
   the length is stored in *len. */
static const char *input_base(const char *path, int strip_txt, int *len)
{
    size_t end = strlen(path), start;

    while (end > 1 && path[end - 1] == '/') end--;
    for (start = end; start > 0 && path[start - 1] != '/'; start--);
    if (strip_txt && ends_with_txt(path + start, end - start)) end -= 4;
    *len = (int)(end - start);
    return path + start;
}




//End Written by claude

/* Evaluates one line. offset = characters in the file before this line.
   Returns 0 and stores the value in *result, a positive error position,
   or -1 if out of memory. */
static long long evaluate_line(const char *line, size_t len, long long offset, Number *result){
    Token *tokens = tokenize(line, len, offset);
    Parser parser;

    if (tokens == NULL) return -1;
    parser.tokens = tokens;
    parser.pos = 0;
    parser.error = 0;

    *result = expr(&parser);

    /* Parsed a full expression but the line continues, e.g. an extra ')' */
    if (parser.error == 0 && tokens[parser.pos].type != TOK_END) {
        parser.error = tokens[parser.pos].position;
    }
    free(tokens);
    return parser.error;
}

/* Reads one line from fp into a new buffer *line (without the '\n'),
   storing its length in *len. Returns 1 if a line was read, 0 at end of
   file, -1 if out of memory. Caller frees *line. */
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
    if (c == EOF && n == 0) {   /* nothing left to read */
        free(buf);
        return 0;
    }
    buf[n] = '\0';
    *line = buf;
    *len = n;
    return 1;
}

/* True for lines that produce no output: blank lines and comment lines
   (first non-blank character is '#'). */
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

/* Evaluates in_path line by line and writes one result per expression.
   Positions count every character of the file (including '\n' and
   comment lines). On the first error the output is replaced by
   "ERROR:<pos>" only. Returns 0 on success (also when the file contained
   an error), -1 if the files couldn't be read/written. */
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
    /* Binary mode so "\r\n" isn't shortened to "\n" on Windows:
       every character in the file must be counted. */
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
        offset += (long long)len + 1;   /* +1 for the '\n' */
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
        /* Throw away any partial results: reopen (truncates the file). */
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

    /* Default output folder in the current folder: <input_base>_name_id */
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
            /* list_txt_files returns bare names, so prepend the folder */
            char *path = build(in, files[i], (int)strlen(files[i]), "");
            if (path == NULL || process_file(path, outdir) != 0) {
                failed = 1;  /* keep going on errors */
            }
            free(path);
            free(files[i]);
        }
        free(files);
        status = failed ? EXIT_FAILURE : EXIT_SUCCESS;
    }

done:
    free(default_dir);
    return status;
}
