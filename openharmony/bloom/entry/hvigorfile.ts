import { hapTasks } from '@ohos/hvigor-ohos-plugin';
import * as fs from 'fs';
import * as path from 'path';

function copyFiles(from: string, into: string, include: RegExp | string) {
  if (!fs.existsSync(from)) {
    console.warn(`[copyTask] 源路径不存在: ${from}`);
    return;
  }
  fs.mkdirSync(into, { recursive: true });
  fs.readdirSync(from).forEach(file => {
    const matched = include instanceof RegExp ? include.test(file) : file === include;
    if (matched) {
      fs.copyFileSync(path.join(from, file), path.join(into, file));
      console.log(`[copyAssets] ${file} -> ${into}`);
    }
  });
}

export default {
  system: hapTasks,
  plugins: [
    {
      pluginId: 'copyAssetsPlugin',
      apply(node: any) {
        const moduleDir = node.getNodeDir().filePath;
        const shaderPath = path.resolve(moduleDir, '../../../shaders/');
        const assetPath  = path.resolve(moduleDir, '../../../assets/');
        const rawfile    = path.resolve(moduleDir, 'src/main/resources/rawfile');

        copyFiles(
          path.resolve(moduleDir, '../../common/res/drawable'),
          path.resolve(moduleDir, 'src/main/resources/base/media'),
          'startIcon.png'
        );
        copyFiles(
          path.resolve(moduleDir, '../../common/res/drawable'),
          path.resolve(moduleDir, '../AppScope/resources/base/media'),
          'startIcon.png'
        );
        copyFiles(path.join(shaderPath, 'glsl/base'),  path.join(rawfile, 'shaders/glsl/base'),          /\.spv$/);
        copyFiles(path.join(shaderPath, 'glsl/bloom'), path.join(rawfile, 'shaders/glsl/bloom'), /^.+\..+$/);
        copyFiles(path.join(assetPath,  'textures'), path.join(rawfile, 'textures'), 'cubemap_space.ktx');
        copyFiles(path.join(assetPath, 'models'), path.join(rawfile, 'models'), 'cube.gltf');
        copyFiles(path.join(assetPath, 'models'), path.join(rawfile, 'models'), 'retroufo.gltf');
        copyFiles(path.join(assetPath, 'models'), path.join(rawfile, 'models'), 'retroufo_glow.gltf');
      }
    }
  ]
}

