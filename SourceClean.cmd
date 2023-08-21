@echo off
@echo Cleaning the source directories

@rmdir /s /q .vs
@rmdir /s /q x64
@rmdir /s /q bin_x64Debug
@rmdir /s /q bin_x64Release
@rmdir /s /q bin_x64UnicodeDebug
@rmdir /s /q bin_x64UnicodeRelease
