生成Linux解决方案，执行以下两步：
1、chmod +x ./generate_Sln.sh
2、./generate_Sln.sh
注意：不这样弄，直接执行：cmake .也能生成。不过不指定生成目录，会导致大量Makefile之类的文件生成在当前目录。