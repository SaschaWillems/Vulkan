"""Check real hybrid shadow pixels, skipping only drivers without ray-query support."""
from pathlib import Path
import subprocess
import sys

renderer, model, artifacts = (Path(value).resolve() for value in sys.argv[1:])


def render(mode):
    output = artifacts / mode
    output.mkdir(parents=True, exist_ok=True)
    process = subprocess.run([str(renderer), '--model', str(model), '--render-mode', mode, '--snapshot'],
                             cwd=output, capture_output=True, text=True, timeout=120)
    log = process.stdout + process.stderr
    if '[validation error]' in log or '[validation warning]' in log:
        raise RuntimeError(log)
    if process.returncode:
        if 'Requested hardware render mode unavailable:' in log:
            print(log)
            sys.exit(77)
        raise RuntimeError(log)
    with (output / 'output.ppm').open('rb') as file:
        assert file.readline() == b'P6\n'
        width, height = map(int, file.readline().split())
        assert file.readline() == b'255\n'
        pixels = file.read()
    offset = ((height // 2) * width + width // 2) * 3
    return sum(pixels[offset:offset+3]) / 3


hybrid = render('hybrid')
raster = render('raster')
assert raster > hybrid + 40, f'Expected occluder shadow on receiver, got raster={raster}, hybrid={hybrid}'
print(f'Ray-query contact shadow verified: raster={raster:.1f}, hybrid={hybrid:.1f}')
