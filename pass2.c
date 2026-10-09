/*
 * pass2.c — SIC/XE 兩階段組譯器（系統程式期末專題）
 *
 * 以課程提供的範本（tokenizer：ASM_getc / ASM_token、opcode table、組譯器架構）為基礎延伸完成。
 * Pass 1：逐行配置位址（LOC）並建立符號表；Pass 2：依 Format 1–4 與定址模式（# @ ,X）
 * 計算 PC-relative / Base-relative 位移、產生目的碼，輸出 H / T / M / E 記錄的 Object Program。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LEN_SYMBOL (20)
#define TRUE (1)
#define FALSE (0)

#define FMT0 0x00 /* SIC Assembler Directive */
#define FMT1 0x01 /* 格式 1 */
#define FMT2 0x02 /* 格式 2 */
#define FMT3 0x04 /* 格式 3 */
#define FMT4 0x08 /* 格式 4 */
#define OP_BYTE 0x101
#define OP_WORD 0x102
#define OP_RESB 0x103
#define OP_RESW 0x104
#define OP_BASE 0x105
#define OP_NOBASE 0x106
#define OP_START 0x107
#define OP_END 0x108

#define ADDR_SIMPLE 0x01
#define ADDR_IMMEDIATE 0x02
#define ADDR_INDIRECT 0x04
#define ADDR_INDEX 0x08

#define LINE_EOF (-1)
#define LINE_COMMENT (-2)
#define LINE_ERROR (0)
#define LINE_CORRECT (1)

int ASM_token(char *buf);    /* 從文件中獲得一个記號 */

FILE *ASM_fp;
int ASM_buf;
int ASM_flag = FALSE;
char DELIMITER[] = " ,\t\r\n";
int LEN_DELIMITER = sizeof(DELIMITER) - 1; /* 減去最後的字符 '\0' */
char SPECIAL[] = "#@+*,.";                 /* 在 DELIMITER 和 SPECIAL 中的逗號 */
int LEN_SPECIAL = sizeof(SPECIAL) - 1;     /* 減去最後的字符 '\0' */

int ASM_getc(void) {
    if (ASM_flag) /* ASM_buf 包含一個字符 */
    {
        ASM_flag = FALSE;
        return (ASM_buf);
    }
    return (fgetc(ASM_fp));
}

void ASM_ungetc(int c) {
    ASM_flag = TRUE;
    ASM_buf = c;
}

int is_delimiter(int c, int space_in_operand) {
    int i;
    for (i = 0; i < LEN_DELIMITER; i++) {
        if (c == DELIMITER[i]) {
            if (!space_in_operand)
                return TRUE;
            else
                return FALSE;
        }
    }
    return FALSE;
}

int is_special(int c) {
    int i;

    for (i = 0; i < LEN_SPECIAL; i++)
        if (c == SPECIAL[i])
            return TRUE;
    return FALSE;
}

int ASM_token(char *buf)
{
    int c;
    int len;
    int space_in_operand = FALSE;

    buf[0] = '\0';
    /* 跳過空白字符 */
    c = ASM_getc();
    while (c == ' ' || c == '\t') {
        c = ASM_getc();
    }
    if (c == EOF)
        return (EOF);
    /* 現在 c 是符號的第一個字符 */
    if (is_special(c)) {
        buf[0] = c;
        buf[1] = '\0';
        len = 1;
    } else if (c == '\r' || c == '\n') {
        c = ASM_getc();
        if (c != '\r' && c != '\n')
            ASM_ungetc(c);
        buf[0] = '\n';
        buf[1] = '\0';
        len = 1;
    } else {
        for (len = 0; !is_delimiter(c, space_in_operand) && c != EOF; c = ASM_getc()) {
            if (len < LEN_SYMBOL - 1) {
                if (buf[0] == 'C' || buf[0] == 'X') {
                    if (buf[1] == '\'')
                        space_in_operand = TRUE;
                    else
                        space_in_operand = FALSE;
                }
                if (space_in_operand && c == '\'')
                    space_in_operand = FALSE;
                buf[len] = c;
                len++;
            }
        }
        buf[len] = '\0';

        if (c != ' ' && c != '\t')
            ASM_ungetc(c);
    }
    return (len);
}


typedef struct {
    char op[LEN_SYMBOL];
    unsigned fmt;
    unsigned code;
} Instruction;

Instruction *is_opcode(char *op);
/* 如果找到，返回指向 OPTAB[i] 的指針；否則返回 NULL */

Instruction OPTAB[] = {
    {"ADD", FMT3 | FMT4, 0x18},
    {"ADDF", FMT3 | FMT4, 0x58},
    {"ADDR", FMT2, 0x90},
    {"AND", FMT3 | FMT4, 0x40},
    {"BASE", FMT0, OP_BASE},
    {"BYTE", FMT0, OP_BYTE},
    {"CLEAR", FMT2, 0xB4},
    {"COMP", FMT3 | FMT4, 0x28},
    {"COMPF", FMT3 | FMT4, 0x88},
    {"COMPR", FMT2, 0xA0},
    {"DIV", FMT3 | FMT4, 0x24},
    {"DIVF", FMT3 | FMT4, 0x64},
    {"DIVR", FMT2, 0x9C},
    {"END", FMT0, OP_END},
    {"FIX", FMT1, 0xC4},
    {"FLOAT", FMT1, 0xC0},
    {"HIO", FMT1, 0xF4},
    {"J", FMT3 | FMT4, 0x3C},
    {"JEQ", FMT3 | FMT4, 0x30},
    {"JGT", FMT3 | FMT4, 0x34},
    {"JLT", FMT3 | FMT4, 0x38},
    {"JSUB", FMT3 | FMT4, 0x48},
    {"LDA", FMT3 | FMT4, 0x00},
    {"LDB", FMT3 | FMT4, 0x68},
    {"LDCH", FMT3 | FMT4, 0x50},
    {"LDF", FMT3 | FMT4, 0x70},
    {"LDL", FMT3 | FMT4, 0x08},
    {"LDS", FMT3 | FMT4, 0x6C},
    {"LDT", FMT3 | FMT4, 0x74},
    {"LDX", FMT3 | FMT4, 0x04},
    {"LPS", FMT3 | FMT4, 0xD0},
    {"MUL", FMT3 | FMT4, 0x20},
    {"MULF", FMT3 | FMT4, 0x60},
    {"MULR", FMT2, 0x98},
    {"NOBASE", FMT0, OP_NOBASE},
    {"NORM", FMT1, 0xC8},
    {"OR", FMT3 | FMT4, 0x44},
    {"RD", FMT3 | FMT4, 0xD8},
    {"RESB", FMT0, OP_RESB},
    {"RESW", FMT0, OP_RESW},
    {"RMO", FMT2, 0xAC},
    {"RSUB", FMT3 | FMT4, 0x4C},
    {"SHIFTL", FMT2, 0xA4},
    {"SHIFTR", FMT2, 0xA8},
    {"SIO", FMT1, 0xF0},
    {"SSK", FMT3 | FMT4, 0xEC},
    {"STA", FMT3 | FMT4, 0x0C},
    {"START", FMT0, OP_START},
    {"STB", FMT3 | FMT4, 0x78},
    {"STCH", FMT3 | FMT4, 0x54},
    {"STF", FMT3 | FMT4, 0x80},
    {"STI", FMT3 | FMT4, 0xD4},
    {"STL", FMT3 | FMT4, 0x14},
    {"STS", FMT3 | FMT4, 0x7C},
    {"STSW", FMT3 | FMT4, 0xE8},
    {"STT", FMT3 | FMT4, 0x84},
    {"STX", FMT3 | FMT4, 0x10},
    {"SUB", FMT3 | FMT4, 0x1C},
    {"SUBF", FMT3 | FMT4, 0x5C},
    {"SUBR", FMT2, 0x94},
    {"SVC", FMT2, 0xB0},
    {"TD", FMT3 | FMT4, 0xE0},
    {"TIO", FMT1, 0xF8},
    {"TIX", FMT3 | FMT4, 0x2C},
    {"TIXR", FMT2, 0xB8},
    {"WD", FMT3 | FMT4, 0xDC},
    {"WORD", FMT0, OP_WORD}
};

int LEN_OPTAB = sizeof(OPTAB) / sizeof(Instruction);

Instruction *is_opcode(char *op)
/* 如果找到，返回指向 OPTAB[i] 的指針；否則返回 NULL */
{
    int begin = 0;
    int end = LEN_OPTAB - 1;
    int mid, c;
    char buf[LEN_SYMBOL];
    char *p;

    /* 將小寫轉換為大寫 */
    for (c = 0, p = op; *p != '\0'; c++, p++) {
        if (*p <= 'z' && *p >= 'a')
            buf[c] = *p - 'a' + 'A';
        else
            buf[c] = *p;
    }
    buf[c] = '\0';

    while (begin <= end) {
        mid = (begin + end) / 2;
        c = strcmp(buf, OPTAB[mid].op);
        if (c == 0)
            return &(OPTAB[mid]);
        else if (c < 0)
            end = mid - 1;
        else
            begin = mid + 1;
    }
    return NULL;
}

const char *sicxeOnly[] = {
    "ADDR",
    "CLEAR",
    "COMPR",
    "DIVR",
    "LDB",
    "LDS",
    "LDT",
    "MULR",
    "RMO",
    "SHIFTL",
    "SHIFTR",
    "STB",
    "STS",
    "STT",
    "SUBR",
    "TIXR"
};

unsigned isSicXeInstruction(const char *instruction) {
    for (int i = 0; i < sizeof(sicxeOnly) / sizeof(sicxeOnly[0]); i++) {
        if (strcmp(instruction, sicxeOnly[i]) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

typedef struct
{
    char symbol[LEN_SYMBOL];
    char op[LEN_SYMBOL];
    char operand1[LEN_SYMBOL];
    char operand2[LEN_SYMBOL];
    unsigned code;
    unsigned fmt;
    unsigned loc;
    unsigned addressing;
} LINE;

typedef struct 
{
    int addr;
    unsigned useBASE;
} DISP;


LINE line_arr[100];
LINE m_arr[100];
int m_count = 0;
int BASE = 0;
int line_len = 0;
FILE *fptr;

int process_line(LINE *line);
/* 返回 LINE_EOF、LINE_COMMENT、LINE_ERROR、LINE_CORRECT 和指令信息 *line*/

void init_LINE(LINE *line) {
    line->symbol[0] = '\0';
    line->op[0] = '\0';
    line->operand1[0] = '\0';
    line->operand2[0] = '\0';
    line->code = 0x0;
    line->fmt = 0x0;
    line->loc = 0x0;
    line->addressing = ADDR_SIMPLE;
}

int process_line(LINE *line)
/* 返回 LINE_EOF、LINE_COMMENT、LINE_ERROR、LINE_CORRECT */
{
    char buf[LEN_SYMBOL];
    int c;
    int state;
    int ret;
    Instruction *op;

    c = ASM_token(buf); /* 獲取一行的第一個標記 */
    if (c == EOF)
        return LINE_EOF;
    else if ((c == 1) && (buf[0] == '\n')) /* 空行 */
        return LINE_COMMENT;
    else if ((c == 1) && (buf[0] == '.')) /* 註釋行 */
    {
        do {
            c = ASM_token(buf);
        } while ((c != EOF) && (buf[0] != '\n'));
        return LINE_COMMENT;
    } else {
        init_LINE(line);
        ret = LINE_ERROR;
        state = 0;
        while (state < 8) {
            switch (state) {
            case 0:
            case 1:
            case 2:
                op = is_opcode(buf);
                if ((state < 2) && (buf[0] == '+')) /* + */
                {
                    line->fmt = FMT4;
                    state = 2;
                } else if (op != NULL) /* 指令 */
                {
                    strcpy(line->op, op->op);
                    line->code = op->code;
                    state = 3;
                    if (line->fmt != FMT4) {
                        line->fmt = op->fmt & (FMT1 | FMT2 | FMT3);
                    }
                } else if (state == 0) /* 符號 */
                {
                    strcpy(line->symbol, buf);
                    state = 1;
                }
                break;
            case 3:
                if (line->fmt == FMT1 || line->code == 0x4C) /* 不需要運算元 */
                {
                    if (c == EOF || buf[0] == '\n') {
                        ret = LINE_CORRECT;
                        state = 8;
                    } else /* 註釋 */
                    {
                        ret = LINE_CORRECT;
                        state = 7;
                    }
                } else {
                    if (c == EOF || buf[0] == '\n') {
                        ret = LINE_ERROR;
                        state = 8;
                    } else if (buf[0] == '@' || buf[0] == '#') {
                        if (buf[0] == '#') {
                            line->addressing = ADDR_IMMEDIATE;
                        } else {
                            line->addressing = ADDR_INDIRECT;
                        }
                        state = 4;
                    } else /* 獲取一個符號 */
                    {
                        op = is_opcode(buf);
                        strcpy(line->operand1, buf);
                        state = 5;
                    }
                }
                break;
            case 4:
                op = is_opcode(buf);
                strcpy(line->operand1, buf);
                state = 5;
                break;
            case 5:
                if (c == EOF || buf[0] == '\n') {
                    ret = LINE_CORRECT;
                    state = 8;
                } else if (buf[0] == ',') {
                    state = 6;
                } else /* 註釋 */
                {
                    ret = LINE_CORRECT;
                    state = 7; /* 跳過行中的後續標記 */
                }
                break;
            case 6:
                if (c == EOF || buf[0] == '\n') {
                    ret = LINE_ERROR;
                    state = 8;
                } else /* 獲取一個符號 */
                {
                    op = is_opcode(buf);
                    if (line->fmt == FMT2) {
                        strcpy(line->operand2, buf);
                        ret = LINE_CORRECT;
                        state = 7;
                    } else if ((c == 1) && (buf[0] == 'x' || buf[0] == 'X')) {
                        // line->addressing = line->addressing | ADDR_INDEX;
                        line->addressing = ADDR_INDEX;
                        ret = LINE_CORRECT;
                        state = 7; /* 跳過行中的後續標記 */
                    }
                }
                break;
            case 7: /* 跳過 '\n' || EOF 之後的標記 */
                if (c == EOF || buf[0] == '\n')
                    state = 8;
                break;
            }
            if (state < 8)
                c = ASM_token(buf); /* 獲取下一個標記 */
        }
        return ret;
    }
}

int addloc(LINE *line) {
    int next_loc = 0;
    char buf_op[LEN_SYMBOL];
    if (line->fmt == FMT4) {  // 如果指令格式是4
        next_loc = 4;  // 下一個位置為4
    } else if (line->code == OPTAB[5].code) {  // 如果指令是BYTE
        if (line->operand1[0] == 'C') {
            next_loc = strlen(line->operand1) - 3;  // 下一個位置為字串長度減去3
        } else {
            next_loc = (strlen(line->operand1) - 3) / 2;  // 下一個位置為字串長度減去3再除以2
        }
    } else if (line->code == OPTAB[38].code) {  // 如果指令是WORD
        next_loc = atoi(line->operand1);  // 下一個位置為操作數的數值
    } else if (line->code == OPTAB[39].code) {  // 如果指令是RESW
        next_loc = atoi(line->operand1) * 3;  // 下一個位置為操作數的數值乘以3
    } else if (line->code == OPTAB[4].code) {  // 如果指令是START
        next_loc = 0;  // 下一個位置為0
    } else {
        if (line->fmt == FMT2) {  // 如果指令格式是2
            next_loc = 2;  // 下一個位置為2
        } else {
            next_loc = 3;  // 其他情況下，下一個位置為3
        }
    }
    return next_loc;
}

int find_symtab(LINE input_line, int line_count) {
    for (int i = 0; i < 100; i++) {
        if (strcmp(input_line.operand1, line_arr[i].symbol) == 0 && line_count != i) {
            return line_arr[i].loc;  // 返回符號表中對應符號的地址
        }
    }
    return -1;  // 如果找不到對應符號，返回-1
}

unsigned is_symbol(int line_count) {
    for (int i = 0; i < line_len; i++) {
        if (strcmp(line_arr[line_count].operand1, line_arr[i].symbol) == 0 && line_count != i) {
            return 1;  // 如果指令有符號操作數，返回1
        }
    }
    return 0;  // 如果指令沒有符號操作數，返回0
}

DISP sicxe_find_disp(LINE line, int line_count) {
    DISP disp;
    disp.useBASE = 0;
    disp.addr = 0;
    int symtab_loc = find_symtab(line, line_count);  // 查找符號表中對應符號的地址
    if (line.fmt == FMT4) {  // 如果指令格式是4
        if (symtab_loc == -1) {
            disp.addr = atoi(line.operand1);  // 如果找不到對應符號，使用操作數的數值
        } else {
            disp.addr = symtab_loc;  // 如果找到對應符號，使用符號的地址
            m_arr[m_count++] = line;  // 將指令添加到修改記憶體位置的陣列中
        }
    } else {
        if (line.addressing == ADDR_SIMPLE) {  // 如果指令的位址模式是Simple
            if (symtab_loc == -1) {
                disp.addr = 0;  // 如果找不到對應符號，位址為0
            }
        }
        if (line.addressing == ADDR_IMMEDIATE) {  // 如果指令的位址模式是Immediate
            if (symtab_loc == -1) {
                disp.addr = atoi(line.operand1);  // 如果找不到對應符號，位址為操作數的數值
            }
        }
        if (line.addressing == ADDR_INDIRECT) {  // 如果指令的位址模式是Indirect
            if (symtab_loc == -1) {
                disp.addr = 0;  // 如果找不到對應符號，位址為0
            }
        }
        if (symtab_loc != -1) {
            disp.addr = symtab_loc - line_arr[line_count + 1].loc;  // 計算位移量
            if (disp.addr < -2048 || disp.addr > 2047) {
                disp.addr = symtab_loc - BASE;  // 如果位移量超出範圍，使用基底相對位址
                disp.useBASE = TRUE;  // 設置使用基底寄存器
            }
        }
    }
    if (strcmp(line.op, "LDB") == 0) {  // 如果指令是LDB
        BASE = symtab_loc;  // 設置基底寄存器為符號的地址
    }
    return disp;
}


void sicxe_objcode(LINE line, int line_count) {
    unsigned opni_hex;
    unsigned xbpe_hex;
    char opni_char[2];
    char xbpe_char[2];
    char disp_char[2];

    switch (line.fmt) {
    case FMT0:
        if (strcmp(line.op, "BYTE") == 0) {
            if (line.operand1[0] == 'C') {
                for (int i = 2; i < strlen(line.operand1) - 1; i++) {
                    fprintf(fptr,"%02X", line.operand1[i]);  // 輸出字串中每個字元的ASCII碼
                }
            } else if (line.operand1[0] == 'X') {
                for (int i = 2; i < strlen(line.operand1) - 1; i++) {
                    fprintf(fptr,"%c", line.operand1[i]);  // 輸出字串中每個字元
                }
            }
        } else if (strcmp(line.op, "WORD") == 0) {
            fprintf(fptr,"%06X", atoi(line.operand1));  // 輸出操作數的十六進位表示
        } else if (strcmp(line.op, "RSUB") == 0) {
            fprintf(fptr,"4F0000");  // 固定輸出RSUB指令的目的碼
        }
        break;
    case FMT2:
        if (strcmp(line.op, "CLEAR") == 0) {
            fprintf(fptr,"B4");  // 輸出CLEAR指令的目的碼
        } else if (strcmp(line.op, "TIXR") == 0) {
            fprintf(fptr,"B8");  // 輸出TIXR指令的目的碼
        } else if (strcmp(line.op, "COMPR") == 0) {
            fprintf(fptr,"A0");  // 輸出COMPR指令的目的碼
        }
        if (strcmp(line.operand1, "A") == 0) {
            fprintf(fptr,"0");  // 輸出第一個操作數的目的碼
        } else if (strcmp(line.operand1, "X") == 0) {
            fprintf(fptr,"1");
        } else if (strcmp(line.operand1, "L") == 0) {
            fprintf(fptr,"2");
        } else if (strcmp(line.operand1, "B") == 0) {
            fprintf(fptr,"3");
        } else if (strcmp(line.operand1, "S") == 0) {
            fprintf(fptr,"4");
        } else if (strcmp(line.operand1, "T") == 0) {
            fprintf(fptr,"5");
        } else if (strcmp(line.operand1, "F") == 0) {
            fprintf(fptr,"6");
        } else {
            fprintf(fptr,"0");
        }

        if (strcmp(line.operand2, "A") == 0) {
            fprintf(fptr,"0");  // 輸出第二個操作數的目的碼
        } else if (strcmp(line.operand2, "X") == 0) {
            fprintf(fptr,"1");
        } else if (strcmp(line.operand2, "L") == 0) {
            fprintf(fptr,"2");
        } else if (strcmp(line.operand2, "B") == 0) {
            fprintf(fptr,"3");
        } else if (strcmp(line.operand2, "S") == 0) {
            fprintf(fptr,"4");
        } else if (strcmp(line.operand2, "T") == 0) {
            fprintf(fptr,"5");
        } else if (strcmp(line.operand2, "F") == 0) {
            fprintf(fptr,"6");
        } else {
            fprintf(fptr,"0");
        }
        break;
    case FMT3:
    case FMT4:
        if (strcmp(line.op, "RSUB") == 0) {
            fprintf(fptr,"4F0000");  // 固定輸出RSUB指令的目的碼
            break;
        }
        unsigned usePC = TRUE;
        opni_hex = line.code;
        xbpe_hex = 2;

        if (line.addressing == ADDR_INDIRECT)
            opni_hex += 2;
        if (line.addressing == ADDR_INDEX)
            opni_hex += 3;
        if (line.addressing == ADDR_SIMPLE)
            opni_hex += 3;
        if (line.addressing == ADDR_IMMEDIATE) {
            opni_hex += 1;
            if (is_symbol(line_count) == 0) {
                usePC = FALSE;  // 如果操作數不是符號，則不使用PC相對位址
            }
        }
        // addr
        if (line.addressing == ADDR_INDEX) {
            xbpe_hex += 8;  // 設置位址模式為Indexing
        }
            

        // fmt 
        if (line.fmt == FMT4) {
            xbpe_hex += 1;  // 設置格式為Format 4
            usePC = FALSE;  // 不使用PC相對位址
        }
        
        DISP disp = sicxe_find_disp(line, line_count);
        if (disp.useBASE) {
            xbpe_hex += 4;  // 設置位址模式為Base-relative
            usePC = FALSE;  // 不使用PC相對位址
        }
        if (!usePC) {
            xbpe_hex -= 2;  // 不使用PC相對位址
        }
        if (disp.addr >= -2048 && disp.addr < 0) {
            disp.addr += 4096;  // 將位移量調整為正數
        }
        if (line.fmt == FMT4) {
            fprintf(fptr,"%02X%1X%05X", opni_hex, xbpe_hex, disp.addr);  // 輸出目的碼
        } else if (line.fmt == FMT3) {
            fprintf(fptr,"%02X%1X%03X", opni_hex, xbpe_hex, disp.addr);  // 輸出目的碼
        }

        break;
    default:
        break;
    }
}

void sic_objcode(LINE line, int line_count) {
    int disp = find_symtab(line, line_count);  // 尋找符號表中的位移值
    if (strcmp(line.op, "BYTE") == 0) {
        if (line.operand1[0] == 'C') {
            for (int i = 2; i < strlen(line.operand1) - 1; i++) {
                fprintf(fptr,"%02X", line.operand1[i]);  // 輸出字串中每個字元的ASCII碼
            }
        } else if (line.operand1[0] == 'X') {
            for (int i = 2; i < strlen(line.operand1) - 1; i++) {
                fprintf(fptr,"%c", line.operand1[i]);  // 輸出字串中每個字元
            }
        }
    } else if (strcmp(line.op, "WORD") == 0) {
        fprintf(fptr,"%06X", atoi(line.operand1));  // 輸出操作數的十六進位表示
    } else if (strcmp(line.op, "RSUB") == 0) {
        fprintf(fptr,"4C0000");  // 固定輸出RSUB指令的目的碼
    } else if (line.addressing >= ADDR_INDEX) {
        fprintf(fptr,"%02X%04X", line.code, disp + 32768);  // 輸出帶有索引的目的碼
    } else {
       fprintf(fptr,"%02X%04X", line.code, disp);  // 輸出一般的目的碼
    }
}

void header(LINE line, int start_loc, int program_len) {
    fprintf(fptr,"H%-6s%06X%06X\n", line_arr[1].symbol, start_loc, program_len);  // 輸出Header Record
}

int find_nextline(int line_count) {
    int texter_len = 0;
    for (int i = line_count; texter_len <= 27; i++) {
        if (strcmp(line_arr[i].op, "BYTE") == 0) {
            if (line_arr[i].operand1[0] == 'C') {
                texter_len += strlen(line_arr[i].operand1) - 3;  // 計算BYTE指令的長度
            } else if (line_arr[i].operand1[0] == 'X') {
                texter_len += (strlen(line_arr[i].operand1) - 3) / 2;  // 計算BYTE指令的長度
            }
        }
        if (strcmp(line_arr[i].op, "WORD") == 0) {
            texter_len += 3;  // 計算WORD指令的長度
        }
        if (line_arr[i].fmt == FMT2) 
            texter_len += 2;  // 計算格式2指令的長度
        else if (line_arr[i].fmt == FMT3)
            texter_len += 3;  // 計算格式3指令的長度
        else if (line_arr[i].fmt == FMT4)
            texter_len += 4;  // 計算格式4指令的長度
        if (strcmp(line_arr[i].op, "RESB") == 0 || strcmp(line_arr[i].op, "RESW") == 0) {
            return texter_len;  // 如果遇到RESB或RESW指令，則返回已計算的長度
        }
        if (i > line_len) {
            return texter_len;  // 如果已經超過行數，則返回已計算的長度
        }
    }
    return texter_len;
}

int main() {
    // pass 1
    int i, c, line_count, line_loc, last_line_loc = 0;
    int start_loc = 0;
    int program_len = 0;
    char buf[LEN_SYMBOL];
    line_loc = 0;
    LINE line;
    fptr = fopen("output.txt","w");
    ASM_fp = fopen("input.txt","r");
    for (line_count = 1; (c = process_line(&line)) != LINE_EOF; line_count++) {
        if (line_count == 1) {
            if (strcmp(line.op, "START") == 0) {
                line_loc = strtol(line.operand1, NULL, 16);  // 轉換起始位址為十進位數字
            } else {
                line_loc = 0;
            }
            start_loc = line_loc;
            last_line_loc = line_loc;
            line.loc = line_loc;
            line_arr[line_count] = line;
        } else if (strcmp(line.op, "END") == 0) {
            line_loc = last_line_loc;
            line.loc = line_loc;
            line_arr[line_count] = line;
            line_loc += 1;
        } else if (c != LINE_ERROR && c != LINE_COMMENT) {
            last_line_loc = line_loc;
            line.loc = line_loc;
            line_arr[line_count] = line;
            line_loc += addloc(&line);  // 計算下一行指令的位址增量
        }
    }
    program_len = line_loc - start_loc;  // 計算程式的長度
    fclose(ASM_fp);
    line_len = line_count;
    // is sicxe
    unsigned sicxe = FALSE;
    for (int i = 0; i < line_len; i++) {
        if (isSicXeInstruction(line_arr[i].op)) {
            sicxe = TRUE;
            break;
        }
    }
    // pass 2
    header(line_arr[1], start_loc, program_len);  // 輸出Header Record
    if (sicxe) {
        int texter_len = 0;
        int text_count = 0;
        for (int i = 1; i < line_count; i++) {
            if (line_arr[i].fmt == FMT0 && strcmp(line_arr[i].op, "BYTE") != 0 && strcmp(line_arr[i].op, "WORD") != 0) {
                continue;
            }
            if (texter_len == 0) {
                texter_len = find_nextline(i);  // 計算下一行指令的長度
                fprintf(fptr,"T%06X", line_arr[i].loc);  // 輸出Text Record的起始位址
                fprintf(fptr,"%02X", texter_len);  // 輸出Text Record的長度
            }
            sicxe_objcode(line_arr[i], i);  // 輸出指令的目的碼
            if (strcmp(line_arr[i].op, "BYTE") == 0) {
                if (line_arr[i].operand1[0] == 'C') {
                    text_count += strlen(line_arr[i].operand1) - 3;  // 計算字串長度
                } else if (line_arr[i].operand1[0] == 'X') {
                    text_count += (strlen(line_arr[i].operand1) - 3) / 2;  // 計算十六進位數字長度
                }
            }
            if (line_arr[i].fmt == FMT2) 
                text_count += 2;
            else if (line_arr[i].fmt == FMT3)
                text_count += 3;
            else if (line_arr[i].fmt == FMT4)
                text_count += 4;
            if (text_count >= texter_len) {  // 若Text Record已滿，則換行
                fprintf(fptr,"\n");
                text_count = 0;
                texter_len = 0;
            }
        }
        for (int i = 0; i < m_count; i++) {
            fprintf(fptr,"M%06X05\n", m_arr[i].loc + 1);  // 輸出Modification Record
        }
        fprintf(fptr,"E%06X\n", start_loc);  // 輸出End Record
    } else {
        int texter_len = 0;
        int text_count = 0;
        for (int i = 1; i < line_count; i++) {
            if (line_arr[i].fmt == FMT0 && strcmp(line_arr[i].op, "BYTE") != 0 && strcmp(line_arr[i].op, "WORD") != 0) {
                continue;
            }
            if (texter_len == 0) {
                texter_len = find_nextline(i);  // 計算下一行指令的長度
                fprintf(fptr,"T%06X", line_arr[i].loc);  // 輸出Text Record的起始位址
                fprintf(fptr,"%02X", texter_len);  // 輸出Text Record的長度
            }
            sic_objcode(line_arr[i], i);  // 輸出指令的目的碼
            if (strcmp(line_arr[i].op, "BYTE") == 0) {
                if (line_arr[i].operand1[0] == 'C') {
                    text_count += strlen(line_arr[i].operand1) - 3;  // 計算字串長度
                } else if (line_arr[i].operand1[0] == 'X') {
                    text_count += (strlen(line_arr[i].operand1) - 3) / 2;  // 計算十六進位數字長度
                }
            } else {
                text_count += 3;  // WORD指令長度固定為3
            }
            if (text_count >= texter_len) {  // 若Text Record已滿，則換行
                fprintf(fptr,"\n");
                text_count = 0;
                texter_len = 0;
            }
        }
        fprintf(fptr,"E%06X\n", start_loc);  // 輸出End Record
    }
    fclose(fptr);
}
