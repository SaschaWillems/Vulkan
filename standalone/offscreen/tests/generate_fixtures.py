"""Generate self-contained glTF/GLB regression assets without third-party Python packages."""
import base64
import copy
import json
from pathlib import Path
import struct
import sys
import zlib

directory = Path(sys.argv[1])
directory.mkdir(parents=True, exist_ok=True)


def chunk(kind, data):
    return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))


png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 2, 2, 8, 6, 0, 0, 0))
png += chunk(b'IDAT', zlib.compress(bytes([0, 255, 64, 16, 255, 32, 255, 64, 255,
                                         0, 64, 32, 255, 255, 255, 192, 64, 255])))
png += chunk(b'IEND', b'')
(directory / 'albedo.png').write_bytes(png)


def model(index_type=5123):
    data = bytearray()
    views = []

    def append(payload, stride=None):
        while len(data) % 4:
            data.append(0)
        view = {'buffer': 0, 'byteOffset': len(data), 'byteLength': len(payload)}
        if stride:
            view['byteStride'] = stride
        views.append(view)
        data.extend(payload)
        return len(views) - 1

    positions = append(struct.pack('<9f', -1, -1, 0, 1, -1, 0, 0, 1, 0))
    uv = append(struct.pack('<6f', 0, 1, 1, 1, 0.5, 0))
    colors = append(bytes([255, 128, 128, 255, 128, 255, 128, 255, 128, 128, 255, 255]))
    indices = append(struct.pack('<3H' if index_type == 5123 else '<3I', 0, 1, 2))
    asset = {
        'asset': {'version': '2.0'}, 'scene': 0,
        'scenes': [{'nodes': [0, 2]}],
        'nodes': [{'translation': [2, 3, 4], 'children': [1]},
                  {'mesh': 0, 'translation': [1, 0, 0], 'scale': [2, 1, 1]},
                  {'mesh': 1, 'matrix': [-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -2, 0, 0, 1]}],
        'meshes': [{'primitives': [{'attributes': {'POSITION': 0, 'TEXCOORD_0': 1, 'COLOR_0': 2},
                                   'indices': 3, 'material': 0}]},
                   {'primitives': [{'attributes': {'POSITION': 0, 'TEXCOORD_0': 1}, 'material': 1}]}],
        'accessors': [
            {'bufferView': positions, 'componentType': 5126, 'count': 3, 'type': 'VEC3',
             'min': [-1, -1, 0], 'max': [1, 1, 0]},
            {'bufferView': uv, 'componentType': 5126, 'count': 3, 'type': 'VEC2'},
            {'bufferView': colors, 'componentType': 5121, 'count': 3, 'type': 'VEC4', 'normalized': True},
            {'bufferView': indices, 'componentType': index_type, 'count': 3, 'type': 'SCALAR'}],
        'materials': [{'pbrMetallicRoughness': {'baseColorFactor': [0.8, 0.6, 0.4, 1],
                        'metallicFactor': 0.3, 'roughnessFactor': 0.7,
                        'baseColorTexture': {'index': 0}, 'metallicRoughnessTexture': {'index': 0}}},
                      {'doubleSided': True, 'pbrMetallicRoughness': {'metallicFactor': 0,
                        'baseColorFactor': [0.1, 0.7, 0.9, 1]}}],
        'textures': [{'source': 0, 'sampler': 0}],
        'samplers': [{'wrapS': 33071, 'wrapT': 33648, 'minFilter': 9987, 'magFilter': 9729}],
        'images': [{'uri': 'albedo.png'}], 'bufferViews': views,
    }
    return asset, data


def write(name, asset, data):
    asset = copy.deepcopy(asset)
    asset['buffers'] = [{'byteLength': len(data), 'uri': name + '.bin'}]
    (directory / (name + '.bin')).write_bytes(data)
    (directory / (name + '.gltf')).write_text(json.dumps(asset))


def glb(name, asset, data):
    asset = copy.deepcopy(asset)
    data = bytearray(data)
    while len(data) % 4:
        data.append(0)
    asset['bufferViews'].append({'buffer': 0, 'byteOffset': len(data), 'byteLength': len(png)})
    asset['images'] = [{'bufferView': len(asset['bufferViews']) - 1, 'mimeType': 'image/png'}]
    data.extend(png)
    asset['buffers'] = [{'byteLength': len(data)}]
    text = json.dumps(asset).encode()
    text += b' ' * (-len(text) % 4)
    data += b'\0' * (-len(data) % 4)
    body = struct.pack('<II', len(text), 0x4E4F534A) + text
    body += struct.pack('<II', len(data), 0x004E4942) + data
    (directory / name).write_bytes(struct.pack('<III', 0x46546C67, 2, len(body) + 12) + body)


asset, binary = model()
write('hierarchy', asset, binary)
glb('hierarchy.glb', asset, binary)
a32, b32 = model(5125)
glb('indices32.glb', a32, b32)
embedded = copy.deepcopy(asset)
embedded['buffers'] = [{'byteLength': len(binary), 'uri': 'data:application/octet-stream;base64,' + base64.b64encode(binary).decode()}]
embedded['images'][0]['uri'] = 'data:image/png;base64,' + base64.b64encode(png).decode()
(directory / 'embedded.gltf').write_text(json.dumps(embedded))

missing = copy.deepcopy(asset)
missing['images'][0]['uri'] = 'absent.png'
write('missing-image', missing, binary)
bad = copy.deepcopy(asset)
bad['bufferViews'][0]['byteLength'] = 4
write('truncated-accessor', bad, binary)
bad = copy.deepcopy(asset)
bad['nodes'][1]['children'] = [0]
write('cycle', bad, binary)
bad_data = bytearray(binary)
struct.pack_into('<H', bad_data, asset['bufferViews'][3]['byteOffset'], 99)
write('bad-index', asset, bad_data)

sparse = copy.deepcopy(asset)
sparse_data = bytearray(binary)
while len(sparse_data) % 4:
    sparse_data.append(0)
offset = len(sparse_data)
sparse_data.extend(bytes([0, 1, 2, 0]))
sparse['bufferViews'].append({'buffer': 0, 'byteOffset': offset, 'byteLength': 3})
sparse['accessors'][0].pop('bufferView')
sparse['accessors'][0]['sparse'] = {'count': 3, 'indices': {'bufferView': len(sparse['bufferViews']) - 1, 'componentType': 5121},
                                    'values': {'bufferView': 0}}
write('sparse', sparse, sparse_data)

interleaved = copy.deepcopy(asset)
interleaved_data = bytearray(binary)
while len(interleaved_data) % 4:
    interleaved_data.append(0)
offset = len(interleaved_data)
interleaved_data.extend(struct.pack('<18f', -1, -1, 0, 0, 0.5, 0, 1, -1, 0, 0, 0.5, 0, 0, 1, 0, 0, 0.5, 0))
interleaved['bufferViews'].append({'buffer': 0, 'byteOffset': offset, 'byteLength': 72, 'byteStride': 24})
interleaved['accessors'][0]['bufferView'] = len(interleaved['bufferViews']) - 1
interleaved['accessors'].append({'bufferView': len(interleaved['bufferViews']) - 1, 'byteOffset': 12,
                               'componentType': 5126, 'count': 3, 'type': 'VEC3'})
interleaved['meshes'][0]['primitives'][0]['attributes']['NORMAL'] = len(interleaved['accessors']) - 1
write('interleaved', interleaved, interleaved_data)

uv_transform = copy.deepcopy(asset)
uv_transform['extensionsUsed'] = ['KHR_texture_transform']
uv_transform['extensionsRequired'] = ['KHR_texture_transform']
uv_transform['meshes'][0]['primitives'][0]['attributes']['TEXCOORD_1'] = 1
uv_transform['materials'][0]['pbrMetallicRoughness']['baseColorTexture']['extensions'] = {
    'KHR_texture_transform': {'offset': [0.1, 0.2], 'scale': [0.5, 0.75], 'texCoord': 1}}
write('uv-transform', uv_transform, binary)

tiny = copy.deepcopy(asset)
tiny['nodes'] = [{'mesh': 0, 'scale': [0.00001, 0.00001, 0.00001]}]
tiny['scenes'] = [{'nodes': [0]}]
write('tiny-scale', tiny, binary)

alpha = copy.deepcopy(asset)
alpha['materials'][0]['alphaMode'] = 'MASK'
alpha['materials'][0]['alphaCutoff'] = 0.25
alpha['materials'][1]['alphaMode'] = 'BLEND'
alpha['materials'][1]['pbrMetallicRoughness']['baseColorFactor'][3] = 0.5
write('alpha', alpha, binary)
untextured = copy.deepcopy(asset)
untextured['materials'][0]['pbrMetallicRoughness'].pop('baseColorTexture')
untextured['materials'][0]['pbrMetallicRoughness'].pop('metallicRoughnessTexture')
write('untextured', untextured, binary)
metal = copy.deepcopy(untextured)
metal['materials'][0]['pbrMetallicRoughness']['metallicFactor'] = 1
metal['materials'][0]['pbrMetallicRoughness']['roughnessFactor'] = 0.1
write('metal', metal, binary)
depth = copy.deepcopy(asset)
depth['nodes'] = [{'mesh': 0, 'translation': [0, 0, 0.5]}, {'mesh': 1, 'translation': [0, 0, -0.5]}]
depth['scenes'] = [{'nodes': [0, 1]}]
for mesh in depth['meshes']:
    mesh['primitives'][0]['attributes'].pop('COLOR_0', None)
depth['materials'] = [{'pbrMetallicRoughness': {'baseColorFactor': color, 'metallicFactor': 0, 'roughnessFactor': 1}}
                      for color in ([0.9, 0.02, 0.02, 1], [0.02, 0.02, 0.9, 1])]
write('depth', depth, binary)
# A receiver at z=0 and a small offset occluder along the directional light.
shadow_positions = [-1,-1,0, 1,-1,0, 1,1,0, -1,1,0,
                    -0.4,0.2,0.3, 0,0.2,0.3, -0.2,0.6,0.3]
shadow_data = struct.pack('<21f', *shadow_positions) + struct.pack('<9H',0,1,2,0,2,3,4,5,6)
shadow = {'asset': {'version':'2.0'}, 'scene':0, 'scenes':[{'nodes':[0]}], 'nodes':[{'mesh':0}],
          'meshes':[{'primitives':[{'attributes':{'POSITION':0},'indices':1,'material':0}]}],
          'materials':[{'doubleSided':True, 'pbrMetallicRoughness':{'baseColorFactor':[0.8,0.8,0.8,1],
                        'metallicFactor':0,'roughnessFactor':1}}],
          'bufferViews':[{'buffer':0,'byteOffset':0,'byteLength':84},
                         {'buffer':0,'byteOffset':84,'byteLength':18}],
          'accessors':[{'bufferView':0,'componentType':5126,'count':7,'type':'VEC3',
                        'min':[-1,-1,0],'max':[1,1,0.3]},
                       {'bufferView':1,'componentType':5123,'count':9,'type':'SCALAR'}]}
write('rt-shadow',shadow,shadow_data)
(directory / 'invalid.gltf').write_text('{broken')
print(f'Generated glTF fixtures in {directory}')
