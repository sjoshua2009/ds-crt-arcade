#!/bin/sh
# 用法: check_size.sh <cols> <rows> <游戏键或空>
# 游戏键只负责进入对应游戏，第一帧渲染即足够验证布局；
# timeout 兜底，避免管道 EOF 后游戏在轮询循环里空转不退出。
BIN=${BIN:-./build/linux/arm64/release/lobby}
COLS=$1
ROWS=$2
KEY=${3:-q}
printf '%s' "$KEY" | TUI_COLS=$COLS TUI_ROWS=$ROWS timeout 2 $BIN 2>/dev/null > .lob.out
node -e '
const fs = require("fs");
const data = fs.readFileSync(".lob.out", "utf8");
const re = /\x1b\[(\d+);(\d+)H/g;
let m, maxr = 0, maxc = 0;
while ((m = re.exec(data)) !== null) {
  if (+m[1] > maxr) maxr = +m[1];
  if (+m[2] > maxc) maxc = +m[2];
}
const cols = +process.argv[1], rows = +process.argv[2];
const ok = maxr <= rows && maxc <= cols;
console.log(process.argv[3] + " @ " + cols + "x" + rows + ": maxr=" + maxr + " maxc=" + maxc + " -> " + (ok ? "OK" : "!! 溢出 !!"));
process.exit(ok ? 0 : 1);
' "$COLS" "$ROWS" "大厅"
rm -f .lob.out
