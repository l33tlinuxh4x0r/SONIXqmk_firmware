import subprocess
import os
import sys

BOARDS = [
    'redragon/k552/rev2',
    'redragon/k556',
    'keychron/k2',
    'keychron/k4/rgb/v1',
    'keychron/k4/rgb/v2',
    'akko/3061_rgb']

error = False
for kb in BOARDS:
    if subprocess.run(f"qmk compile -kb {kb} -km all -j{os.cpu_count()}", shell=True).returncode != 0:
        error = True
if error:
    sys.exit(1)
