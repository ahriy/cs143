#!/usr/bin/python
#-*- coding:utf-8 -*-
import os
import sys
import subprocess
import time
import logging

ROOT = os.getcwd() + "/.."
CASE_DIR = ROOT + "/examples"
CASEFILE = "case.list"
MYDIR = ""
case_list = open(CASEFILE).readlines()
golden_result = {}
my_result = {}
def run_cmd(cmd, timeout=None):
    try:
        exec_status = -1
        exec_err = ""
        exec_result = ""
        # deprecated，无法提供超时功能
        # exec_status, exec_result = subprocess.getstatusoutput(cmd)

        try:
            # shell=True 因为需要使用通道和字符串子命令，所以默认打开
            with subprocess.Popen(cmd,  shell=True, universal_newlines=True, stderr=subprocess.STDOUT,stdout=subprocess.PIPE) as process:
                # 主要超时控制
                if timeout:
                    to_count = 0
                    while True:
                        # interval sec: 5 因为还有很多long time command，超时间隔不需要太细
                        time.sleep(0.001)
                        to_count += 0.001
                        if to_count > timeout:
                            # 回归到subprocess的超时处理
                            process.kill()
                            raise subprocess.TimeoutExpired(process.args, timeout, output=None,
                                             stderr=None)
                        if process.poll() is None:
                            continue
                        else:
                            break

                try:
                    # 完成后获取结果
                    stdout, stderr = process.communicate(input)
                # 此处是subprocess的异常处理，直接使用
                except subprocess.TimeoutExpired:
                    process.kill()
                    stdout, stderr = process.communicate()
                    raise subprocess.TimeoutExpired(process.args, timeout, output=stdout,
                                         stderr=stderr)
                except:
                    process.kill()
                    process.wait()
                    raise
                retcode = process.poll()
                if retcode:
                    raise subprocess.CalledProcessError(retcode, process.args,
                                             output=stdout, stderr=stderr)
                data = subprocess.CompletedProcess(process.args, retcode, stdout, stderr).stdout
                exec_status = 0
        except subprocess.CalledProcessError as ex:
            data = ex.output
            exec_status = ex.returncode
        if data[-1:] == '\n':
            exec_result = data[:-1]

        if exec_status != 0:
            exec_err = exec_result
            exec_result = None
        return exec_status, exec_result, exec_err
    # 自身封装的异常处理，为了记录和返回统一格式
    except subprocess.TimeoutExpired:
        logging.error("cmd:{} is timeout".format(cmd))
        return -9, None, "timeout"
    except BaseException as ose:
        logging.error("cmd:{} error:{}".format(cmd, ose.__str__()))
        return -1, None, ose.__str__()

def mycmd(cmd):
    print(">>>>" + cmd)
    return run_cmd(cmd, 1)[1]

def get_my(case):
    mycmd("cd {} && make clean && make -j && make lexer".format(MYDIR))
    mycmd("cd {} && ./mycoolc {}".format(MYDIR, CASE_DIR + '/' + case + ".cl"))
    my_result[case] = mycmd("spim {}".format(CASE_DIR + '/' + case + ".s"))
    mycmd("rm {}".format(CASE_DIR + '/' + case + ".s"))

def get_golden(case):
    mycmd("coolc {}".format(CASE_DIR + '/' + case + ".cl"))
    my_result[case] = mycmd("spim {}".format(CASE_DIR + '/' + case + ".s"))
    golden_result[case] = mycmd("spim {}".format(CASE_DIR + '/' + case + ".s"))
    mycmd("rm {}".format(CASE_DIR + '/' + case + ".s"))

def clean_s():
    mycmd("cd {} && make clean".format(MYDIR))
    for case in case_list:
        mycmd("rm {}".format(CASE_DIR + '/' + case + ".s"))

def init():
    for i in range(case_list.__len__()):
        case_list[i] = case_list[i].strip()

if __name__ == "__main__":
    init()
    
    if (len(sys.argv) < 3):
        print("format is python auto_test.py [clean|test] $PAx")
        exit(1)
    cmd = sys.argv[1]
    MYDIR = ROOT + "/assignments/" + sys.argv[2]

    if (cmd == "clean"):
        clean_s()
    elif (cmd == "run"):
        clean_s()
        for case in case_list:
            get_golden(case)
            get_my(case)
        for case in case_list:
            if (golden_result[case] != my_result[case]):
                print("===== case {} result for golden is ===== \n{}".format(case, golden_result[case]))
                print("===== case {} result for my is ===== \n{}".format(case, my_result[case]))
            else:
                print("case {} PASS".format(case))
