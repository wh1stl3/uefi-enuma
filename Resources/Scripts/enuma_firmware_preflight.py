"""Official [firmware] preflight bridge to the pinned uefi-enuma source."""
import os
from pathlib import Path
import runpy

if __name__ == '__main__':
    source = Path(os.environ['ENUMA_SOURCE']).resolve()
    runpy.run_path(str(source / 'Resources/Scripts/enuma_preflight.py'), run_name='__main__')
