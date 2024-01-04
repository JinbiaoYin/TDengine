﻿###################################################################
#           Copyright (c) 2023 by TAOS Technologies, Inc.
#                     All rights reserved.
#
#  This file is proprietary and confidential to TAOS Technologies.
#  No part of this file may be reproduced, stored, transmitted,
#  disclosed or used in any form or by any means other than as
#  expressly provided by the written permission from Jianhui Tao
#
###################################################################

# -*- coding: utf-8 -*-

#
# about system funciton extension
#

import sys
import os
import time
import datetime
import platform
import subprocess

# if windows platform return True
def isWin():
    return platform.system().lower() == 'windows'

#
#  execute programe
#

# wait util execute file finished 
def exe(file):
    return os.system(file)

# execute file and return immediately
def exeNoWait(file):
    print("exe no wait")


# run return output and error
def run(command):
    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    process.wait(3)

    output = process.stdout.read().decode(encoding="gbk")
    error = process.stderr.read().decode(encoding="gbk")

    return output, error


# return list after run
def runRetList(command):
    lines = []
    output,error = run(command)
    return output.splitlines()
