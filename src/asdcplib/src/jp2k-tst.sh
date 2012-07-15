#!/bin/sh
#
# $Id: jp2k-tst.sh,v 1.5 2010/01/05 04:12:15 jhurst Exp $
# Copyright (c) 2007-2009 John Hurst. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# JPEG 2000 tests

mkdir -p ${TEST_FILES}/extract ${TEST_FILES}/plaintext

${BUILD_DIR}/asdcp-test${EXEEXT} -c ${TEST_FILES}/write_test_jp2k.mxf ${TEST_FILES}/${TEST_FILE_PREFIX}
if [ $? -ne 0 ]; then
    exit 1
fi


${BUILD_DIR}/asdcp-test${EXEEXT} -x ${TEST_FILES}/extract/${JP2K_PREFIX} ${TEST_FILES}/write_test_jp2k.mxf
if [ $? -ne 0 ]; then
    exit 1
fi
for file in `ls ${TEST_FILES}/${TEST_FILE_PREFIX}`; do \
  echo "$file"; \
  cmp ${TEST_FILES}/${TEST_FILE_PREFIX}/$file ${TEST_FILES}/extract/$file; \
  if [ $? -ne 0 ]; then \
    exit 1; \
  fi; \
done


#${BUILD_DIR}/j2c-test${EXEEXT} ${TEST_FILES}/${TEST_FILE_PREFIX}/MM_2k_XYZ_000000.j2c
#if [ $? -ne 0 ]; then
#    exit 1
#fi
