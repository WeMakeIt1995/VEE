#!/bin/bash

# 假设你的程序名为 myapp.exe
APP_NAME="build/qemu-system-arm.exe"
OUTPUT_DIR="vee-dist"

# 创建输出目录
mkdir -p "$OUTPUT_DIR"

# 复制主程序
cp "$APP_NAME" "$OUTPUT_DIR/"

# 使用 ntldd 查找并复制依赖
# ntldd -R "$APP_NAME" | grep -i mingw64 | awk '{print $3}' | xargs -I{} cp "{}" "$OUTPUT_DIR/"
ldd "$APP_NAME" | grep -i mingw64 | awk '{print $3}' | xargs -I{} cp "{}" "$OUTPUT_DIR/"

# 复制其他必要资源（如图片、配置文件等）
# cp -r assets "$OUTPUT_DIR/"

echo "打包完成，输出目录: $OUTPUT_DIR"
