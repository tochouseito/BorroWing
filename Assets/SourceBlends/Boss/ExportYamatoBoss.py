from pathlib import Path
import bpy
ROOT = Path(__file__).resolve().parents[3]
SOURCE_DIR = ROOT / 'Assets' / 'SourceBlends' / 'Boss'
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete()
for obj_path in SOURCE_DIR.glob('boss_yamato_*.obj'):
    bpy.ops.wm.obj_import(filepath=str(obj_path))
bpy.ops.wm.save_as_mainfile(filepath=str(SOURCE_DIR / 'BorroWing_YamatoBoss.blend'))
