from pathlib import Path
import json,struct

def load(directory):
 return [json.loads((directory/name).read_text()) for name in ('interface.json','program.json','root.json','backends.json')]
def arrays_initial(directory,interface):
 return {a['buffer']:bytearray((directory/a['initial_data']).read_bytes()) for a in interface['arrays']}
def call(key):return bytes((23,0,160))+struct.pack('<H',key)+b'\x01'
