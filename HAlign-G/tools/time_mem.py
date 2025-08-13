# Author  : zhai1xiao
# Date    : 2022-12-29
# Function: The class of processTimer
# Taken from
# https://stackoverflow.com/questions/13607391/subprocess-memory-usage-in-python

import psutil, time, sys
import subprocess
import time, os, sys
import pandas as pd

class ProcessTimer:
  def __init__(self,command):
    self.command = command
    self.execution_state = False

  def execute(self):
    self.max_vms_memory = 0
    self.max_rss_memory = 0

    self.t1 = None
    self.t0 = time.time()
    self.p = subprocess.Popen(self.command, shell=True)
    self.execution_state = True

  def poll(self):
    if not self.check_execution_state():
      return False

    self.t1 = time.time()

    try:
      pp = psutil.Process(self.p.pid)

      #obtain a list of the subprocess and all its descendants
      descendants = list(pp.children(recursive=True))
      descendants = descendants + [pp]

      rss_memory = 0
      vms_memory = 0

      #calculate and sum up the memory of the subprocess and all its descendants
      for descendant in descendants:
        try:
          mem_info = descendant.memory_info()

          rss_memory += mem_info[0]
          vms_memory += mem_info[1]
        except psutil.Error:
          # sometimes a subprocess descendant will have terminated between the time
          # we obtain a list of descendants, and the time we actually poll this
          # descendant's memory usage.
          pass
      self.max_vms_memory = max(self.max_vms_memory,vms_memory)
      self.max_rss_memory = max(self.max_rss_memory,rss_memory)

    except psutil.Error:
      return self.check_execution_state()


    return self.check_execution_state()


  def is_running(self):
    return psutil.pid_exists(self.p.pid) and self.p.poll() == None

  def check_execution_state(self):
    if not self.execution_state:
      return False
    if self.is_running():
      return True
    self.executation_state = False
    self.t1 = time.time()
    return False

  def close(self,kill=False):
    try:
      pp = psutil.Process(self.p.pid)
      if kill:
        pp.kill()
      else:
        pp.terminate()
    except psutil.Error:
      pass


if __name__ == "__main__":
    # 获取命令行参数
    command = sys.argv[1]

    # 拆分参数1
    outfile = command.split()[-2]

    ptimer = ProcessTimer([command])

    try:
        ptimer.execute()
        #poll as often as possible; otherwise the subprocess might
        # "sneak" in some extra memory usage while you aren't looking
        while ptimer.poll():
            time.sleep(.5)
    finally:
        #make sure that we don't leave the process dangling?
        ptimer.close()

    with open(outfile, 'a') as file:
        file.write(f'return code: {ptimer.p.returncode}\n')
        file.write(f'time: {ptimer.t1 - ptimer.t0}\n')
        file.write('max_vms_memory:' + str({ptimer.max_vms_memory / (1024 * 1024)}) + " MB\n")
        file.write('max_rss_memory:' + str({ptimer.max_rss_memory / (1024 * 1024)}) + " MB\n")
    # print('return code:',ptimer.p.returncode)
    # print('time:',ptimer.t1 - ptimer.t0)
    # print('max_vms_memory:' + str({ptimer.max_vms_memory / (1024 * 1024)}) + " MB")
    # print('max_rss_memory:' + str({ptimer.max_rss_memory / (1024 * 1024)}) + " MB")