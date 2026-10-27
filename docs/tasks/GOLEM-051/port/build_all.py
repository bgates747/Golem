#!/usr/bin/env python3
from build import build
from build_support import build_support
if __name__=='__main__':
    for profile in ('raw','resident','portable','stock-matrix'):build(target=True,profile=profile)
    build_support()
