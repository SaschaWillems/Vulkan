"""Check actual PBR pixels and depth occlusion using generated glTF assets."""
from pathlib import Path
import subprocess
import sys

renderer, fixtures, artifacts = map(lambda value: Path(value).resolve(), sys.argv[1:])


def render(name):
    output = artifacts / name
    output.mkdir(parents=True, exist_ok=True)
    process = subprocess.run([str(renderer), '--model', str(fixtures / (name + '.gltf')), '--snapshot'],
                             cwd=output, capture_output=True, text=True, timeout=45)
    if process.returncode or '[validation error]' in process.stderr:
        raise RuntimeError(process.stdout + process.stderr)
    with (output / 'output.ppm').open('rb') as file:
        assert file.readline() == b'P6\n'
        width, height = map(int, file.readline().split())
        assert file.readline() == b'255\n'
        pixels = file.read()
    assert len(pixels) == width * height * 3
    return width, height, pixels


_, _, textured = render('hierarchy')
_, _, untextured = render('untextured')
_, _, metal = render('metal')
# Sample across the frame instead of comparing every byte of three 2K images.
assert sum(a != b for a, b in zip(textured[::16], untextured[::16])) > 1000, 'PBR textures did not affect pixels'
assert sum(a != b for a, b in zip(untextured[::16], metal[::16])) > 1000, 'Metallic/roughness factors did not affect pixels'
width, height, pixels = render('depth')
offset = ((height // 2) * width + width // 2) * 3
r, g, b = pixels[offset:offset + 3]
assert r > b * 2 and r > 60, f'Far blue mesh overwrote near red mesh: {(r, g, b)}'
print('PBR texture/factor influence and depth occlusion passed')
