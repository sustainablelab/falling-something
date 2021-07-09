#!/usr/bin/python
# -*- coding: utf-8 -*-
import pathlib
with open("headers-windows.txt") as fin:
    with open("headers-posix.txt", mode="w") as fout:
        for line in fin:
            for s in line.split():
                if '.h' in s:
                    fout.write(str(pathlib.PurePosixPath(s)))
                    fout.write("\n")
