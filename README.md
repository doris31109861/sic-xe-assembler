# SIC/XE Assembler

> 系統程式期末專題｜逢甲大學資訊工程學系（成績 91）｜C

[中文](#中文) | [English](#english)

---

## 中文

以 C 語言實作 **SIC/XE** 架構的兩階段組譯器（Two-Pass Assembler）：讀入組合語言原始碼，Pass 1 計算位址並建立符號表，Pass 2 產生目的碼，並輸出完整的 Object Program（H / T / M / E 記錄）。

> 本專題以課程提供的範本程式（tokenizer、opcode table 與組譯器架構）為基礎延伸完成。

### 功能

- **詞彙分析**：`ASM_getc`、`ASM_ungetc`、`ASM_token` 處理分隔符號與特殊字元 `# @ + * , .`
- **指令格式**：支援 Format 1、2、3、4（`+` 延伸格式）
- **定址模式**：簡單、立即（`#`）、間接（`@`）、索引（`,X`）
- **組譯指引**：`START`、`END`、`BYTE`、`WORD`、`RESB`、`RESW`、`BASE`、`NOBASE`
- **Pass 1**：為每一行分配位址（LOC）並建立符號表
- **Pass 2**：計算 PC-relative / Base-relative 位移並產生目的碼
- **Object Program**：輸出 Header、Text、Modification、End 記錄
- **錯誤檢查**：產生目的碼前檢查重複定義與未定義的符號，印出行號；有錯誤時不產生輸出檔

### 編譯與執行

```bash
gcc pass2.c -o assembler
./assembler examples/input.txt output.obj   # 指定輸入、輸出檔
./assembler                                 # 不給參數時讀 input.txt、輸出 output.txt
```

錯誤訊息範例（把第 5 行的標籤改成重複的 `FIRST`、第 7 行用了沒定義的 `ZERO`）：

```
第 5 行：符號 FIRST 重複定義（第 2 行已定義）
第 7 行：未定義的符號 ZERO（指令 COMP）
第 10 行：未定義的符號 CLOOP（指令 J）
共 3 個錯誤，未產生 output.obj
```

### 執行結果（`examples/output.txt`）

```
HCOPY  000000001077
T0000001D17202D69202D4B1010360320262900003320074B10105D3F2FEC032010
...
M00000705
E000000
```

### 學到的東西

- 組譯器如何把助憶碼轉成機器碼，以及為什麼需要兩階段處理前向參照
- 指令編碼（n/i/x/b/p/e 位元）與相對定址
- 用 C 語言撰寫小型詞彙分析器與剖析器

---

## English

A two-pass assembler for the **SIC/XE** architecture, written in C. Pass 1 assigns addresses and builds the symbol table; Pass 2 generates object code and writes a complete object program (H / T / M / E records).

> Extended from the course-provided template (tokenizer, opcode table and assembler skeleton).

### Features

- Hand-written lexer handling delimiters and the special characters `# @ + * , .`
- Instruction formats 1, 2, 3 and 4 (`+` extended)
- Simple, immediate (`#`), indirect (`@`) and indexed (`,X`) addressing
- Directives: `START`, `END`, `BYTE`, `WORD`, `RESB`, `RESW`, `BASE`, `NOBASE`
- PC-relative and base-relative displacement
- Header, Text, Modification and End records
- Error checks before code generation: duplicate labels and undefined symbols are reported with line numbers, and no output is written

### Build & Run

```bash
gcc pass2.c -o assembler
./assembler examples/input.txt output.obj   # explicit input and output
./assembler                                 # defaults: input.txt → output.txt
```

### What I learned

- How an assembler turns mnemonics into machine code, and why two passes are needed for forward references
- Instruction encoding (n/i/x/b/p/e bits) and relative addressing
- Writing a small lexer and parser in plain C
