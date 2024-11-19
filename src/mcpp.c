#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define VERSION "20241016"
#define PROC_NAME_LEN 64
#define LINE_LEN 256

long lnum;
char *fname;

char *tmp_name = "temp.out";

char *enabled_chars = "[]()-+*/%$|&^.: \t<>";

char is_far = 0;

int psize, lsize, bits = 16;

char *bp_reg, *sp_reg, *ax_reg;

char oname[256], ename[256], inc_name[256], line[LINE_LEN], pstr[LINE_LEN];
char cond[256], jncond[16];

char paramNamesBuf[8192];
int paramName[128], paramCount, paramNamesPtr;

long lid = 0, lbrk = 0, lcont = 0, lret = 0;

char is_alp(char c) { return (c == '_') || ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')); }
char is_dig(char c) { return ((c >= '0') && (c <= '9')); }
char is_id(char c) { return is_dig(c) || is_alp(c); }
char is_space(char c) { return (c <= ' ') && (c != 0); }

char *skip_space(char *p) {
    while(is_space(*p)) { p++; }
    return p;
}

void trim_line() {
        char *p1, *p2, c;
        p1 = line;
        p2 = skip_space(line);
        if(p1 != p2) {
                strcpy(line, p2);
        }
        p1 = line;
        while(*p1 != 0) {
                c = *p1;
                if((c == '\'') || (c == '\"')) {
                        p1++;
                        while((*p1 != c) && (*p1 != 0)) {
                                if(*p1 == '\\') {
                                        p1++;
                                }
                                p1++;
                        }
                } else if(c == ';') {
                        *p1 = 0;
                        break;
                } else {
                        if(c < ' ') {
                                *p1 = ' ';
                        }
                        p1++;
                }
        }
        p1--;
        while((*p1 <= ' ') && (p1 >= line)) {
                *p1 = 0;
                p1--;
        }
}

char *next_line(FILE *inf) {
        char *result;
        result = fgets(line, LINE_LEN, inf);
        if(result != NULL) {
                trim_line();
                lnum++;
        }
        return result;
}

char mk_err(char *msg) {
        FILE *f;
        printf("ERROR:%s(%ld): %s\n> %s\n", fname, lnum, msg, line);
        if((f = fopen(ename, "w")) != NULL) {
                fprintf(f, "ERROR:%s(%ld): %s\n%ld %s\n", fname, lnum, msg, lnum, line);
                fclose(f);
        }
        return 1;
}

long next_label() {
        lid++;
        return lid;
}

void def_label(FILE *outf, long labelId) {
        fprintf(outf, "._MC%08x:\n", labelId);
}

void move_to_label(FILE *outf, char *op, long labelId) {
        fprintf(outf, "\t%s\t._MC%08x\n", op, labelId);
}

char *getid(char *p) {
    int i = 0;
    p = skip_space(p);
    while(is_id(*p)) {
        pstr[i] = *p;
        p++;
        i++;
    }
    pstr[i] = 0;
    return p;
}

char *cb_cond_arg(char *p, char *buf) {
        char *p_tmp, *buf_tmp, delim;
        p = skip_space(p);
        while(1) {
                if((*p == 0) || (*p == ';')) {
                        break;
                }
                if(is_id(*p)) {
                        p_tmp = p;
                        buf_tmp = buf;
                        while(is_id(*p)) {
                                *buf = *p;
                                p++;
                                buf++;
                        }
                        *buf = 0;
                        if((strcmp(buf_tmp, "THEN") == 0)   || (strcmp(buf_tmp, "DO") == 0)
                                || (strcmp(buf_tmp, "LE") == 0) || (strcmp(buf_tmp, "GE") == 0)
                                || (strcmp(buf_tmp, "E") == 0) || (strcmp(buf_tmp, "NE") == 0)
                                || (strcmp(buf_tmp, "G") == 0) || (strcmp(buf_tmp, "L") == 0)) {
                                p = p_tmp;
                                buf = buf_tmp;
                                break;
                        }
                } else if(strchr(enabled_chars, *p) != NULL) {
                                *buf = *p;
                                p++;
                                buf++;
                } else if((*p == '\'') || (*p == '\"')) {
                        delim = *p;
                        *buf = *p;
                        p++;
                        buf++;
                        while((*p != delim) && (*p != 0)) {
                                if(*p == '\\') {
                                        *buf = *p;
                                        p++;
                                        buf++;
                                }
                                *buf = *p;
                                p++;
                                buf++;
                        }
                }
        }
        *buf = 0;
        return p;
}

char codeblock(FILE *inf, FILE *outf);

char cb_cond(FILE *inf, FILE *outf, char *p, char *end_lex, char *msg) {
        char *p0;
        char lcond[256], cmpcond[256], rcond[256];
        cond[0] = 0;
        jncond[0] = 0;
        p0 = skip_space(p);
        p = skip_space(getid(p0));
        if(strcmp(pstr, "ZFLAG") == 0) {
                strcpy(jncond, "JNZ");
        } else if(strcmp(pstr, "NZFLAG") == 0) {
                strcpy(jncond, "JZ");
        } else if(strcmp(pstr, "CFLAG") == 0) {
                strcpy(jncond, "JNC");
        } else if(strcmp(pstr, "NCFLAG") == 0) {
                strcpy(jncond, "JC");
        } else if(strcmp(pstr, "OFLAG") == 0) {
                strcpy(jncond, "JNO");
        } else if(strcmp(pstr, "NOFLAG") == 0) {
                strcpy(jncond, "JO");
        } else if(strcmp(pstr, "PFLAG") == 0) {
                strcpy(jncond, "JNP");
        } else if(strcmp(pstr, "NPFLAG") == 0) {
                strcpy(jncond, "JP");
        } else if(strcmp(pstr, "TRUE") == 0) {
        } else if(strcmp(pstr, "ZERO") == 0) {
                p = skip_space(getid(p));
                sprintf(cond, "OR\t%s, %s", pstr, pstr);
                strcpy(jncond, "JNZ");
        } else if(strcmp(pstr, "NZERO") == 0) {
                p = skip_space(getid(p));
                sprintf(cond, "OR\t%s, %s", pstr, pstr);
                strcpy(jncond, "JZ");
        } else {
                p = p0;
                p = cb_cond_arg(p, lcond);
                p = skip_space(getid(p));
                strcpy(cmpcond, pstr);
                p = cb_cond_arg(p, rcond);
                if(strcmp(pstr, "E") == 0) {
                        strcpy(jncond, "JNZ");
                } else if(strcmp(pstr, "NE") == 0) {
                        strcpy(jncond, "JZ");
                } else if(strcmp(pstr, "GE") == 0) {
                        strcpy(jncond, "JL");
                } else if(strcmp(pstr, "LE") == 0) {
                        strcpy(jncond, "JG");
                } else if(strcmp(pstr, "L") == 0) {
                        strcpy(jncond, "JGE");
                } else if(strcmp(pstr, "G") == 0) {
                        strcpy(jncond, "JLE");
                } else {
                        sprintf(lcond, "Invalid compare operator %s", cond);
                        return mk_err(lcond);
                }
                sprintf(cond, "CMP\t%s, %s", lcond, rcond);
        }
        p = skip_space(getid(p));
        if(strcmp(pstr, end_lex) != 0) {
                return mk_err(msg);
        }
        return 0;
}

char cb_while(FILE *inf, FILE *outf, char *p) {
        char r;
        long old_lbrk, old_lcont;

        old_lcont = lcont;
        old_lbrk = lbrk;

        lcont = next_label();
        lbrk = next_label();

        def_label(outf, lcont);

        if(r = cb_cond(inf, outf, p, "DO", "WHILE without DO")) {
                return 1;
        }

        if(strlen(cond) > 0) {
                fprintf(outf, "\t%s\n", cond);
        }
        if(strlen(jncond) > 0) {
                move_to_label(outf, jncond, lbrk);
        }
        if(r = codeblock(inf, outf)) {
                return r;
        }
        move_to_label(outf, "JMP", lcont);
        def_label(outf, lbrk);

        lcont = old_lcont;
        lbrk = old_lbrk;
        return 0;
}

char cb_if(FILE *inf, FILE *outf, char *p) {
        char r;
        long lfalse, lend;

        if(r = cb_cond(inf, outf, p, "THEN", "IF without THEN")) {
                return 1;
        }
        lfalse = next_label();
        if(strlen(cond) > 0) {
                fprintf(outf, "\t%s\n", cond);
        }
        move_to_label(outf, jncond, lfalse);
    r = codeblock(inf, outf);
    if(r == 0) {
                def_label(outf, lfalse);
    } else if(r == -1) {
                /* elseif */

/*
                if(r = cb_cond(inf, outf, p, "THEN", "IF without THEN")) {
                        return 1;
                }
                lfalse = next_label();
                if(strlen(cond) > 0) {
                        fprintf(outf, "\t%s\n", cond);
                }
                move_to_label(outf, jncond, lfalse);
                move_to_label(outf, "JMP", lend = next_label());


                lend = next_label();
                move_to_label(outf, "JMP", lend = next_label());
                def_label(outf, lfalse);
                fprintf(outf, "; begin if else\n");
                r = codeblock(inf, outf);
                fprintf(outf, "; end of if else\n");
                def_label(outf, lend);
*/
                r = 0;
    } else if(r == -2) {
                /* else */
                lend = next_label();
                move_to_label(outf, "JMP", lend = next_label());
                def_label(outf, lfalse);
                r = codeblock(inf, outf);
                def_label(outf, lend);
    }
        return r;
}

char codeblock(FILE *inf, FILE *outf) {
        char *p, r;
    while(next_line(inf) != NULL) {
                if(line[0] == 0) {
                        continue;
                }
        p = skip_space(getid(line));
        if(strcmp(pstr, "RETURN") == 0) {
            move_to_label(outf, "JMP", lret);
                        if(*p != 0) {
                                return mk_err("Missing ; at end of operator");
                        }
        } else if(strcmp(pstr, "IF") == 0) {
                        if(r = cb_if(inf, outf, p)) {
                                return r;
                        }
        } else if(strcmp(pstr, "WHILE") == 0) {
                        if(r = cb_while(inf, outf, p)) {
                                return r;
                        }
        } else if(strcmp(pstr, "BREAK") == 0) {
                        if(lbrk == 0) {
                                return mk_err("BREAK whithout WHILE/END");
                        }
                        if(*p != 0) {
                                return mk_err("Missing ; at end of operator");
                        }
            move_to_label(outf, "JMP", lbrk);
        } else if(strcmp(pstr, "CONTINUE") == 0) {
                        if(lcont == 0) {
                                return mk_err("CONTINUE whithout WHILE/END");
                        }
                        if(*p != 0) {
                                return mk_err("Missing ; at end of operator");
                        }
            move_to_label(outf, "JMP", lcont);
        } else if(strcmp(pstr, "END") == 0) {
                        if(*p != 0) {
                                return mk_err("Missing ; at end of operator");
                        }
            return 0;
        } else if(strcmp(pstr, "ELSEIF") == 0) {
            return -1;
        } else if(strcmp(pstr, "ELSE") == 0) {
            return -2;
        } else {
                        fprintf(outf, "\t%s\n", line);
        }
    }
        return 0;
}

char check_far(char *p) {
        p = skip_space(getid(p));
        if(strcmp(pstr, "FAR") == 0) {
                is_far = 1;
                if(*p != 0) {
                        return mk_err("Missing ; at end of definition");
                }
                p = skip_space(p + 1);
        } else if(strcmp(pstr, "NEAR") == 0) {
                if(*p != 0) {
                        return mk_err("Missing ; at end of definition");
                }
                p = skip_space(p + 1);
        }
        return *p != 0;
}

char parseParams(char *p) {
    p = skip_space(p);
    if(*p == '(') {
                p = skip_space(p + 1);
                while(1) {
                        p = getid(p);
                        strcpy(paramNamesBuf + paramNamesPtr, pstr);
                        paramName[paramCount] = paramNamesPtr;
                        paramCount++;
                        paramNamesPtr += strlen(pstr) + 1;
                        p = skip_space(p);
                        if(*p != ',') {
                                break;
                        }
                        p = skip_space(p + 1);
                }
                if(*p != ')') {
                        return mk_err("Require ) at end procedure parameters definition.");
                }
                p = skip_space(p + 1);
        }
        p = skip_space(p);
        return  check_far(p);
}

void defParams(FILE *outf) {
        int rsize, psize, i;
        if(bits == 16) {
                psize = 2;
                rsize = is_far ? 4 : 2;
        } else if(bits == 32) {
                psize = 4;
                rsize = is_far ? 8 : 4;
        }
        for(i = 0; i < paramCount; i++) {
                fprintf(outf, "%%define\t%s\tBP+%d\n", paramNamesBuf + paramName[i], (paramCount - i - 1) * psize + rsize);
        }
        fprintf(outf, "%%define\tRESULT\t%s\n", ax_reg);
}

void undefParams(FILE *outf) {
        int i;
        for(i = 0; i < paramCount; i++) {
                fprintf(outf, "%%undef\t%s\n", paramNamesBuf + paramName[i]);
        }
        fprintf(outf, "%%undef\tRESULT\n");
}

char def_proc(char *p, FILE *inf, FILE *glob, FILE *outf, FILE *incf) {
    char name[PROC_NAME_LEN];
    char is_global = 0;
    char *retName;
    char r;

        paramCount = 0;
        paramNamesPtr = 0;

    psize = lsize = 0;
    is_far = 0;
    p = getid(p);
    strcpy(name, pstr);
    p = skip_space(p);
    if(*p == '*') {
        fprintf(glob, "\tglobal %s\n", name);
        fprintf(incf, "\textern\t%s\t; %s\n", name, skip_space(line));
        p++;
    }
    p = skip_space(p);

    fprintf(outf, "; %s\n", skip_space(line));
        if(r = parseParams(p)) {
                return r;
        }
    fprintf(outf, "%s:\n", name);
        defParams(outf);
    fprintf(outf, "\tPUSH\t%s\n", bp_reg);
    fprintf(outf, "\tMOV\t%s, %s\n", bp_reg, sp_reg);
    if(lsize) {
                fprintf(outf, "\tSUB\t%s, %d\n", sp_reg, lsize);
        }
        
        lret = next_label();

        if(r = codeblock(inf, outf)) {
                return r;
        }
        
        def_label(outf, lret);
    fprintf(outf, "\tMOV\t%s, %s\n", sp_reg, bp_reg);
    fprintf(outf, "\tPOP\t%s\n", bp_reg);
    if(bits == 16) {
                psize = paramCount * 2;
        } else if(bits == 32) {
                psize = paramCount * 4;
        }
    retName = is_far ? "RETF" : "RETN";
    if(psize) {
                fprintf(outf, "\t%s\t%d\n", retName, psize);
        } else {
                fprintf(outf, "\t%s\n", retName);
        }
        undefParams(outf);
    fprintf(outf, "; end %s\n\n", name);
    return 0;
}

char def_struc(char *p, FILE *inf, FILE *glob, FILE *outf, FILE *incf) {
        char is_glob = 0;

        p = getid(p);
        p = skip_space(p);
        if(*p == '*') {
                is_glob = 1;
        }
        
        fprintf(outf, "STRUC %s\n", pstr);
        if(is_glob) {
                fprintf(incf, "\nSTRUC %s\n", pstr);
        }

    while(next_line(inf) != NULL) {
                p = getid(line);
        if(strcasecmp(pstr, "ENDSTRUC") == 0) {
                        break;
                }
                fprintf(outf, "\t%s\n", line);
                if(is_glob) {
                        fprintf(incf, "\t%s\n", line);
                }
        }
        fprintf(outf, "ENDSTRUC\n");
        if(is_glob) {
                fprintf(incf, "ENDSTRUC\n\n");
        }
        return 0;
}

char process(FILE *inf, FILE *glob, FILE *outf, FILE *incf) {
    char *p;
    lnum = 0;
        bits = 16; bp_reg = "BP"; sp_reg = "SP"; ax_reg = "AX";
    fprintf(glob, "%%define\tTRUE\t1\n%%define\tFALSE\t0\n%%define\tNIL\t0\n");
        fprintf(incf, "; Autogenerated file\n");
    while(next_line(inf) != NULL) {
        p = getid(line);
        if(strcmp(pstr, "PROCEDURE") == 0) {
            if(def_proc(p, inf, glob, outf, incf)) {
                return 1;
            }
        } else if(strcasecmp(pstr, "BITS") == 0) {
                        fprintf(outf, "\t%s\n", line);
                        p = getid(p);
                        if(strcmp(pstr, "16") == 0) {
                                bits = 16; bp_reg = "BP"; sp_reg = "SP"; ax_reg = "AX";
                        } else if(strcmp(pstr, "32") == 0) {
                                bits = 32; bp_reg = "EBP"; sp_reg = "ESP"; ax_reg = "EAX";
                        } else {
                                return mk_err("Bits could be 16 or 32");
                        }
        } else if(strcasecmp(pstr, "STRUC") == 0) {
            if(def_struc(p, inf, glob, outf, incf)) {
                return 1;
            }
        } else if(line[0] == '%') {
                        p = getid(line + 1);
                        if(strcasecmp(pstr, "define") == 0) {
                                p = skip_space(getid(p));
                                if(*p == '*') {
                                        *p = ' ';
                                        fprintf(incf, "%s\n", line);
                                }
                                fprintf(outf, "%s\n", line);
                        } else {
                                fprintf(outf, "%s\n", line);
                        }
        } else if(line[0] != 0) {
                        fprintf(outf, "%s\n", line);
        }
    }
    return 0;
}


void copyFile(FILE *dst, FILE *src) {
    char buf[8192];
        if((src != NULL) && (dst != NULL)) {
                while(fgets(buf, 8192, src) != NULL) {
                        fputs(buf, dst);
                }
    }
}

void print_help(char *name) {
        int i;
        i = strlen(name);
        while((name[i] != '\\') && (name[i] != '/')) {
                i--;
        }
        i++;
        printf("MissConstruct PreProcessor for NASM, Version %s\n", VERSION);
        printf("(c) 2024 DosWorld, https://github.com/DosWorld/missconstruct, MIT License\n\n");
        printf("USAGE:\n\t%s inputfile\n", name + i);
        exit(1);
}

char prepareNames(char *f) {
        int i;
        fname = f;
        strcpy(oname, f);
        strcpy(ename, f);
        strcpy(inc_name, f);
        i = strlen(oname);
        while((i >= 0) && (oname[i] != '.') && (oname[i] != '/') && (oname[i] != '\\') && (oname[i] != ':')) {
                i--;
        }
        if(i < 0) {
                i = strlen(f);
                if(i == 0) {
                        strcpy(oname, "out.");
                        strcpy(ename, "out.");
                        strcpy(inc_name, "out.");
                        i = strlen(oname) - 1;
                }
        }
        i++;
        oname[i] = 'a'; ename[i] = 'e'; inc_name[i] = 'i'; i++;
        oname[i] = 's'; ename[i] = 'r'; inc_name[i] = 'n'; i++;
        oname[i] = 'm'; ename[i] = 'r'; inc_name[i] = 'c'; i++;
        oname[i] = 0; ename[i] = 0; inc_name[i] = 0; i++;

        return 0;
}
FILE *myfopen(char *fname, char *acc, char *msg) {
        FILE *r = fopen(fname, acc);
        if(r == NULL) {
                printf(msg, fname);
        }
        return r;
}

int main(int argc, char *argv[]) {
    int r;
    FILE *inf, *outf, *glob, *incf;

        if(argc != 2) {
                print_help(argv[0]);
        }

        if(prepareNames(argv[1])) {
                print_help(argv[0]);
        }
        remove(ename);
        remove(oname);
        remove(inc_name);

        lnum = 0;
    r = 1;
    inf = myfopen(fname, "r", "ERROR:Could not read %s\n");
    glob = myfopen(oname, "w", "ERROR:Could not write %s\n");
    outf = myfopen(tmp_name, "w", "ERROR:Could not write %s\n");
    incf = myfopen(inc_name, "w", "ERROR:Could not write %s\n");
        if((inf != NULL) && (glob != NULL) && (outf != NULL) && (incf != NULL)) {
        r = process(inf, glob, outf, incf);
        fclose(outf);
        outf = fopen(tmp_name, "rb");
        copyFile(glob, outf);
        }
    if(inf != NULL) {
                fclose(inf);
        }
        if(outf != NULL) {
                fclose(outf);
        }
        if(incf != NULL) {
                fclose(incf);
        }
        if(glob != NULL) {
                fclose(glob);
        }
    remove(tmp_name);
    if(r) {
                remove(oname);
                remove(inc_name);
    }
    exit(r);
}
