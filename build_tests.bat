set curdir=%cd%
cd .\test
for %%# in (*.c) do gcc "%%#" -o "%%~n#.exe" -Wall -Werror -Wextra -pedantic -g -std=c99
cd %curdir%
