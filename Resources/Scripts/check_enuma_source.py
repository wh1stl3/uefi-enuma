"""Check imported source hashes and declared Mu gitlinks without OEM data."""
import ast
import hashlib
import json
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[2]
lock = json.loads((ROOT / 'Resources/Enuma/migration.json').read_text())
for item in lock['files'].values():
    path = ROOT / item['destination']
    assert hashlib.sha256(path.read_bytes()).hexdigest() == item['sha256'], item['destination']
for path in (ROOT / 'Resources/Scripts').glob('*enuma*.py'):
    ast.parse(path.read_text(), filename=str(path))
expected = json.loads((ROOT / 'Resources/Enuma/dependencies.json').read_text())['reference_gitlinks']
for name, sha in expected.items():
    actual = subprocess.check_output(['git', '-C', str(ROOT), 'ls-tree', 'HEAD', name], text=True).strip()
    assert actual == '160000 commit ' + sha + '\t' + name, name
print('PASS: imported source hashes, Python syntax and three declared Mu gitlinks; no binary build')
