import subprocess
import os

availableExtensions = [".vert", ".frag", ".comp"]
execLocation = r"D:\Users\Bjorn\Documents\Libraries\Vulkan\1.2.162.1\Bin32\glslc.exe"

#get all the files with the correct extensions
for file in os.listdir(os.getcwd()):
    if file.endswith(tuple(availableExtensions)):
        fileName = file.replace('.', '_')
        subprocess.check_call([execLocation, file, r"-o" + fileName + ".spv"])