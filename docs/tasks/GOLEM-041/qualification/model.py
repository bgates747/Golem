# Qualification uses the maintained standalone reference, not task compiler code.
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[4]/'tests/portable'))
from command_model import reference
