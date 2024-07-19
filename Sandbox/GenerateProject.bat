@echo off
call ..\vendor\Premake\bin\premake5.exe vs2022 --flare_root=../ --file=Build.lua
PAUSE
