bl_info = {
    "name": "QuadWild-BiMDF",
    "author": "Martin Heisterman",
    "version": (0, 0, 1),
    "blender": (3, 5, 0),
    "description": "QuadWild retopology with BiMDF solver",
    "location": "", # example: "View3D > Add > Mesh",
    "warning": "", # used for warning icon and text in addons panel
    "doc_url": "https://github.com/cgg-bern/quadwild-bimdf",
    "tracker_url": "https://github.com/cgg-bern/quadwild-bimdf/issues",
    "support": "COMMUNITY",
    "category": "Mesh",
}

import bpy
import math
import subprocess
import os
from pathlib import Path
from dataclasses import dataclass

RUNNING_WITHOUT_INSTALL = (__name__ == "__main__")

BINDIR = "/Users/mh/tmp/quadwild-bimdf-release/macos/quadwild-bimdf-0.0.1"
CONFDIR = "/Users/mh/github/cgg-bern/quadwild-bimdf/config"

@dataclass
class PrepConfig:
    """Configuration for quadwild prep (remesh + field + trace)"""
    remesh: bool = True
    feature_angle_thresh_rad: float = 45./180 * math.pi

@dataclass
class MainConfig:
    """Configuration for quadwild main (quantize + extract)"""
    alpha: float = 0.01

def popen_prep(mesh_filepath: Path, conf: PrepConfig):
    # TODO: save config to tempfile
    return  subprocess.Popen([get_binary_filepath(),
                              mesh_filepath,
                              "3",
                              os.path.join(CONFDIR, "prep_config", "basic_setup.txt")
                              ])

def popen_main(conf: MainConfig):
    pass

#def set_binary_filepath(self, value):
#    print("set", value)
#    print("self", self)
#    self["quadwild_binary_filepath"] = value

class AddonPrefs(bpy.types.AddonPreferences):
    bl_idname = __name__


    quadwild_binary_filepath: bpy.props.StringProperty(
        name="Path to quadwild binary",
        subtype="FILE_PATH")
        #set=set_binary_filepath)
    # TODO: use setter to validate / grab version

    def draw(self, context):
        layout = self.layout
        layout.label(text="QuadWild-BiMDF preferences")
        if quadwild_binary_works():
            icon = 'NONE'
        else:
            icon = 'ERROR'
        layout.prop(self, "quadwild_binary_filepath", icon=icon)

def get_addon_prefs():
    assert not RUNNING_WITHOUT_INSTALL
    return bpy.context.preferences.addons[__name__].preferences

def get_binary_filepath():
    if RUNNING_WITHOUT_INSTALL:
        return os.getenv("QUADWILD_BINARY_FILEPATH")
    else:
        return get_addon_prefs().quadwild_binary_filepath

def quadwild_binary_works():
    # TODO: get version
    filepath = get_binary_filepath()
    return filepath and os.path.exists(filepath)



class ProgressInfo:
    def __init__(self, area):
        self.area = area

    def update(self, text=None):
        # TODO: show wallclock time elapsed
        if self.area:
            self.area.header_text_set(text)
    

def load_obj(filepath):
    """return object"""
    # It appears that "bpy.ops.import_scene.obj" uses the legacy importer
    x = bpy.ops.wm.obj_import(filepath=str(filepath))

    try:
        return bpy.context.selected_objects[-1]
    except IndexError:
        return None

class QuadWildPropGroup(bpy.types.PropertyGroup):
    scale:          bpy.props.FloatProperty(name='scale',               default=1, min=0, soft_min=0.001)
    preprocess:     bpy.props.BoolProperty (name='Auto-preprocess',     default=True)
    sharp_thresh:   bpy.props.FloatProperty(name='Sharp edge thresh',   default=35/180*math.pi, min=0., max=math.pi, unit='ROTATION')
    alpha:          bpy.props.FloatProperty(name="alpha",               default=0.01, min=0., max=1.)
    align:          bpy.props.BoolProperty (name='Enable alignment',    default=False)
    align_weight:   bpy.props.FloatProperty(name='Alignment weight',    default=0.1, min=0., max=10.)
    smoothing:      bpy.props.BoolProperty (name='Smoothing?',          default=True)
    create_new_obj: bpy.props.BoolProperty (name='Create new object',   default=True)



class QuadWildExecOp(bpy.types.Operator):
    def on_finished(self, context):
        """override this"""
        pass

    def on_abort(self, context):
        """override this"""
        pass

    def on_failure(self, context):
        """override this"""
        pass

    def on_success(self, context):
        """override this"""
        pass

    def get_tmpdir(self, obj):
        p = Path(bpy.app.tempdir) / "quadwild_bimdf" / obj.name
        p.mkdir(parents=True, exist_ok=True)
        return p

    @classmethod
    def poll(self, context):
        return quadwild_binary_works()

    def _finished(self, context):
        print("QW finished")
        self._disable_timer(context)
        self.progress.update(None)
        self.on_finished(context)

    def _abort(self, context):
        print("QW abort")
        self.proc.terminate() # TODO: kill?
        self.on_abort(context)

    def _success(self, context):
        print("QW success")
        self.on_success(context)

    def _failure(self, context):
        print("QW failure")
        self.on_failure(context)


    def _enable_timer(self, context):
        wm = context.window_manager
        self.timer = wm.event_timer_add(0.5, window=context.window)

    def _disable_timer(self, context):
        wm = context.window_manager
        self.timer = wm.event_timer_remove(self.timer)
        self.timer = None

    def modal(self, context, event):
        wm = context.window_manager
        #print("QW modal ev", event.type, repr(event))
        if event.type == 'ESC':
            self.report({'INFO'}, "QuadWild-BiMDF retopo canceled")
            self._abort(context)
            self._finished(context)
            return {'CANCELLED'}

        elif event.type == 'TIMER':
            ret = self.proc.poll()

            if ret is None: # still running
                #print("poll None")
                self.progress.update("QuadWild-BiMDF working..." + str(self.prog))
                self.prog+=1
                return {'RUNNING_MODAL'}
            elif ret == 0:
                self._success(context)
                self._finished(context)
                return {'FINISHED'}
            else:
                self._failure(context)
                self._finished(context)
                return {'CANCELLED'}
        return {'RUNNING_MODAL'}

class QuadWildPrepOp(QuadWildExecOp):
    """QuadWild-BiMDF Retopology: Prep"""          # Use this as a tooltip for menu items and buttons.
    bl_idname = "quadwild_bimdf.prep_op"           # Unique identifier for buttons and menu items to reference.
    bl_label = "Retopologize prep"                 # Display name in the interface.
    bl_options = {'REGISTER', 'UNDO'}              # Enable undo for the operator.



class QuadWildMainOp(QuadWildExecOp):
    """QuadWild Retopology (Bi-MDF version)"""     # Use this as a tooltip for menu items and buttons.
    bl_idname = "quadwild_bimdf.main_op"           # Unique identifier for buttons and menu items to reference.
    bl_label = "Retopologize main"                 # Display name in the interface.
    bl_options = {'REGISTER', 'UNDO'}              # Enable undo for the operator.


    def execute(self, context):
        obj = context.active_object
        wm = context.window_manager
        pg = obj.quadwild_propgrp
        print(pg.alpha)
        print("QW run")
        mesh_filepath = self.get_tmpdir(obj) / "mesh.obj"
        print("QuadWild-BiMDF: saving mesh to be quadrangulated as ", mesh_filepath)
        bpy.ops.wm.obj_export(filepath=str(mesh_filepath),
                              check_existing=False,
                              apply_modifiers=True,
                              export_eval_mode='DAG_EVAL_RENDER',
                              export_selected_objects=True,
                              export_uv=False,
                              export_normals=False,
                              export_materials=False,
                              export_triangulated_mesh=True,
                             )

        self.proc = subprocess.Popen([get_binary_filepath(),
                                      mesh_filepath,
                                      "3",
                                      os.path.join(CONFDIR, "prep_config", "basic_setup.txt")
                                      ])
        self._enable_timer(context)
        self.progress = ProgressInfo(context.area)
        self.prog = 10
        self.progress.update("QuadWild-BiMDF working...")
        wm.modal_handler_add(self)
        return {'RUNNING_MODAL'}


    def on_finished(self, context):
        obj = context.active_object
        tmpdir = self.get_tmpdir(obj)
        filepath = self.get_tmpdir(context.active_object) / "mesh_rem_quadrangulation_smooth.obj"
        #filepath=os.path.join(TEMPDIR, "mesh_rem_p0_0_quadrangulation_smooth.obj")
        obj=load_obj(filepath)
        if obj:
            obj.name="retopo result"
        else:
            self.report({'ERROR'}, "QuadWild-BiMDF failure, cannot open output file")



class QuadWildPanel(bpy.types.Panel):
    """QuadWild Retopology (Bi-MDF version)"""     # Use this as a tooltip for menu items and buttons.
    bl_idname = "VIEW3D_PT_quadwild_bimdf"              # Unique identifier for buttons and menu items to reference.
    bl_label = "Retopologize with QuadWild-BiMDF"  # Display name in the interface.
    #bl_options = {'REGISTER', 'UNDO'}              # Enable undo for the operator.
    bl_space_type = 'VIEW_3D' # https://docs.blender.org/api/current/bpy_types_enum_items/space_type_items.html#rna-enum-space-type-items
    bl_region_type = 'UI' # https://docs.blender.org/api/current/bpy_types_enum_items/region_type_items.html#rna-enum-region-type-items
    bl_category = "Retopo"

    @classmethod
    def poll(self, context):
        obj = context.active_object
        return obj.type == 'MESH'


    def draw(self, context):
        if not quadwild_binary_works():
            self.layout.label(text="Quadwild binary not found at '{}'".format(get_binary_filepath()))
            return
        ws = context.workspace
        obj = context.active_object
        if not obj:
            return
        col = self.layout.column()
        pg = obj.quadwild_propgrp

        box = col.box()
        box.label(text="QW Settings:")
        #box.prop(ws, "quadwild_binary_filepath")
        box.prop(pg, "scale")
        box.prop(pg, "preprocess")
        box.prop(pg, "sharp_thresh") # TODO: option to take sharp features from blender (edge type, material boundary, ...)
        box.prop(pg, "alpha")
        box.prop(pg, "align")
        r = box.row()
        r.prop(pg, "align_weight")
        r.enabled = pg.align
        col.row().prop(pg, "create_new_obj")
        col.row().operator(QuadWildMainOp.bl_idname)

#def menu_func(self, context):
#    self.layout.operator(QuadWildRetopology.bl_idname)

CLASSES = [
        QuadWildPropGroup,
        QuadWildPrepOp,
        QuadWildMainOp,
        QuadWildPanel,
        AddonPrefs,
        ]
def register():
    print("DEBUG: QuadWild-BiMDF register()")
    for cl in CLASSES:
        bpy.utils.register_class(cl)
    bpy.types.Object.quadwild_propgrp = bpy.props.PointerProperty(type=QuadWildPropGroup)
    #bpy.types.WorkSpace.quadwild_binary_filepath = prop_quadwild_binary_filepath
    #bpy.types.VIEW3D_MT_object.append(menu_func)  # Adds the new operator to an existing menu.

def unregister():
    for cl in CLASSES:
        bpy.utils.unregister_class(cl)
    del bpy.types.Object.quadwild_propgrp


if __name__ == "__main__":
    register()

