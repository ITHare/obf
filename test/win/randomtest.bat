@REM Copyright (c) 2018, ITHare.com
@REM All rights reserved.
@REM
@REM Redistribution and use in source and binary forms, with or without
@REM modification, are permitted provided that the following conditions are met:
@REM
@REM * Redistributions of source code must retain the above copyright notice, this
@REM  list of conditions and the following disclaimer.
@REM
@REM * Redistributions in binary form must reproduce the above copyright notice,
@REM   this list of conditions and the following disclaimer in the documentation
@REM   and/or other materials provided with the distribution.
@REM
@REM * Neither the name of the copyright holder nor the names of its
@REM   contributors may be used to endorse or promote products derived from
@REM   this software without specific prior written permission.
@REM
@REM THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
@REM AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
@REM IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
@REM DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
@REM FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
@REM DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
@REM SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
@REM CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
@REM OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
@REM OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

@ECHO OFF
cl /EHsc advapi32.lib ..\randomtestgen.cpp
SET NN=%1
IF NOT .%1 == . GOTO LABEL0
SET NN=1024
:LABEL0

IF NOT ERRORLEVEL 1 GOTO LABEL1
EXIT /B
:LABEL1

randomtestgen.exe %NN% >generatedrandomtest.bat
IF NOT ERRORLEVEL 1 GOTO LABEL1
EXIT /B
:LABEL1

CALL generatedrandomtest.bat



