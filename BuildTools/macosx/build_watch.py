import os, time, sys, subprocess


path_to_watch = sys.argv[1]

before = os.path.getmtime(path_to_watch)

while 1:
    time.sleep (10)
    after = os.path.getmtime(path_to_watch)
    if after != before:
        f = open(path_to_watch, 'r')
        version = f.readline().strip()
        f.close()
        print version
        # call build script
        if os.name == 'nt':
            subprocess.check_call("call_git.bat")
            subprocess.check_call("build.bat")
        else:
            subprocess.Popen(["./auto_build.10.7.sh", version, "10", "1"])
    before = after
