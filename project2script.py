import subprocess
import os

os.chdir("./minet-netclass-w16-proj2")
subprocess.call(["ls"])
subprocess.call(["make", "clean"])
subprocess.call("make")

os.chdir("./bin/")
subprocess.call(["ls"])
subprocess.call(["rm", "device_driver2"])
subprocess.call(["rm", "reader"])
subprocess.call(["rm", "writer"])
subprocess.call(["ln", "-s", "/usr/local/eecs340/device_driver2"])
subprocess.call(["ln", "-s", "/usr/local/eecs340/reader"])
subprocess.call(["ln", "-s", "/usr/local/eecs340/writer"])

os.chdir("..")
subprocess.call(["ls"])
subprocess.call(["./setup.sh"])

os.chdir("./fifos/")
subprocess.call(["ls"])
subprocess.call(["chmod", "a+w", "ether2mon"])
subprocess.call(["chmod", "a+w", "ether2mux"])
