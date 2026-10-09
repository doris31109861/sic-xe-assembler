#!/usr/bin/env bash
# tests/run_tests.sh — 自動比對組譯器輸出與預期結果
#   1. 課程範例（SIC/XE COPY 程式）：Object Program 必須與 examples/output.txt 完全相同
#   2. 錯誤案例：重複標籤與未定義符號，stderr 必須與預期訊息相同、回傳 1、且不產生輸出檔
#   3. 輸入檔不存在：回傳 1
# 用法：bash tests/run_tests.sh [組譯器執行檔，預設 ./assembler]

ASM="${1:-./assembler}"
TMP="$(mktemp -d)"
FAILS=0
pass() { echo "PASS  $1"; }
fail() { echo "FAIL  $1"; FAILS=$((FAILS + 1)); }

# 1) 範例程式
"$ASM" examples/input.txt "$TMP/copy.obj"
if diff <(tr -d '\r' < "$TMP/copy.obj") <(tr -d '\r' < examples/output.txt) > "$TMP/diff.txt"; then
    pass "範例 COPY 程式的 Object Program 與 examples/output.txt 相同"
else
    fail "範例輸出不同："; cat "$TMP/diff.txt"
fi

# 2) 錯誤案例（在暫存資料夾執行，讓訊息中的輸出檔名固定為 out.obj）
cp tests/cases/errors.asm "$TMP/"
(cd "$TMP" && "$OLDPWD/$ASM" errors.asm out.obj 2> errors.out); code=$?
[[ $code -eq 1 ]] && pass "錯誤案例回傳 1" || fail "錯誤案例回傳 $code"
diff <(tr -d '\r' < "$TMP/errors.out") tests/cases/errors.expected > /dev/null \
    && pass "錯誤訊息與行號正確" || { fail "錯誤訊息不同："; cat "$TMP/errors.out"; }
[[ ! -e "$TMP/out.obj" ]] && pass "有錯誤時不產生輸出檔" || fail "有錯誤卻產生了輸出檔"

# 3) 輸入檔不存在
"$ASM" "$TMP/no-such-file.asm" "$TMP/x.obj" 2> /dev/null; code=$?
[[ $code -eq 1 ]] && pass "輸入檔不存在時回傳 1" || fail "輸入檔不存在時回傳 $code"

rm -rf "$TMP"
echo "-----"
[[ $FAILS -eq 0 ]] && echo "全部通過" || { echo "$FAILS 項失敗"; exit 1; }
