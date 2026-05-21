# Notes on ZIP format

> [!WARNING]
>
> This is not a library or a program for managing ZIP archives

## Story

I am not satisfied with the current set of tools that are available for
reading Excel files. I've tried a bunch of them, and every one of them seems
to be not that fast or memory-efficient as I'd like. Nowadays, the most common
Excel file format is .xlsx, which is basically just a ZIP archive, that
contains a bunch of XML files maintaining a specific structure.

This is why I considered to write my own implementation for reading .xlsx-like
files which would be faster and more memory efficient than those I had
encountered so far.

This repository contains notes, that describe what I have learned during
exploration of the ZIP file format. You can also find a small program that
parses central directory entries ([main.c](main.c), [zip.h](zip.h)) and prints
them to stdout. It was not intended for an actual usage by someone else, but,
of course, anyone is free to do anything they want with it. 
