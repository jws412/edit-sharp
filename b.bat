@echo off
cls
(gcc main.c init.c dpi_manager.c memory_manager.c -o a.exe -luser32 -lgdi32 -Werror -Wall -Wextra -pedantic -Wcast-align -Wcast-qual -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wredundant-decls -Wshadow -Wundef -Wno-unused -Wno-variadic-macros -Wno-parentheses -fdiagnostics-show-option -Werror=vla -std=c99 -O0 || GOTO FAIL)
echo Build is successful.
EXIT /B

:FAIL
echo Build failed.
EXIT /B