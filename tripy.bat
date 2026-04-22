@echo off
setlocal
set "ROOT=%~dp0"
set "PYTHONPATH=%ROOT%tripy\src;%PYTHONPATH%"
python -m tripy.cli %*
