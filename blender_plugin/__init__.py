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

ADDON_IDNAME = "quadwild_bimdf"

TEMPDIR = os.path.join(bpy.app.tempdir, "quadwild_bimdf")
BINDIR = "/Users/mh/tmp/quadwild-bimdf-release/macos/quadwild-bimdf-0.0.1"
CONFDIR = "/Users/mh/github/cgg-bern/quadwild-bimdf/config"



class AddonPrefs(bpy.types.AddonPreferences):
    bl_idname = ADDON_IDNAME

    quadwild_binary_filepath: bpy.props.StringProperty(
        name="Path to quadwild binary",
        subtype="FILE_PATH")

    def draw(self, context):
        layout = self.layout
        layout.label(text="QuadWild-BiMDF preferences")
        layout.prop(self, "quadwild_binary_filepath")

def get_addon_prefs(context):
    return context.preferences.addons[ADDON_IDNAME].preferences

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
    x = bpy.ops.wm.obj_import(filepath=filepath)

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

class QuadWildOperator(bpy.types.Operator):
    """QuadWild Retopology (Bi-MDF version)"""     # Use this as a tooltip for menu items and buttons.
    bl_idname = "quadwild_bimdf.op"                # Unique identifier for buttons and menu items to reference.
    bl_label = "Retopologize"                      # Display name in the interface.
    bl_options = {'REGISTER', 'UNDO'}              # Enable undo for the operator.


    @classmethod
    def poll(self, context):
        # TODO: check if we can run (e.g., binary available)
        return True


    def execute(self, context):
        obj = context.active_object
        wm = context.window_manager
        pg = obj.quadwild_propgrp
        print(pg.alpha)
        print("QW run")
        try:
            os.mkdir(TEMPDIR)
        except FileExistsError:
            pass
        filename = "tmp" # we probably want to increase some counter here...
        filepath= os.path.join(TEMPDIR, "mesh.obj")
        print("QuadWild-BiMDF: saving mesh to be quadrangulated as ", filepath)
        bpy.ops.wm.obj_export(filepath=filepath,
                              check_existing=False,
                              apply_modifiers=True,
                              export_eval_mode='DAG_EVAL_RENDER',
                              export_selected_objects=True,
                              export_uv=False,
                              export_normals=False,
                              export_materials=False,
                              export_triangulated_mesh=True,
                             )

        binpath = addon_prefs = get_addon_prefs(context).quadwild_binary_filepath
        print("pref ", binpath)
        self.proc = subprocess.Popen([os.path.join(BINDIR, "quadwild"),
                                      filepath,
                                      "3",
                                      os.path.join(CONFDIR, "prep_config", "basic_setup.txt")
                                      ])
        self.timer = wm.event_timer_add(0.5, window=context.window)
        self.timer = wm.modal_handler_add(self)
        self.progress = ProgressInfo(context.area)
        self.prog = 10
        self.progress.update("QuadWild-BiMDF working...")
        return {'RUNNING_MODAL'}

    def finished(self):
        print("QW finished")
        self.progress.update(None)

        #filepath=os.path.join(TEMPDIR, "mesh_rem_p0_0_quadrangulation_smooth.obj")
        filepath=os.path.join(TEMPDIR, "mesh_rem_quadrangulation_smooth.obj")
        obj=load_obj(filepath)
        if obj:
            obj.name="retopoo"
        else:
            self.report({'ERROR'}, "QuadWild-BiMDF failure, cannot open output file")


        pass

    def abort(self):
        print("QW abort")
        self.proc.terminate() # TODO: kill?
        self.finished()

    def cancel(self, arg):
        print("QW cancel", arg)

    def modal(self, context, event):
        wm = context.window_manager
        #print("QW modal ev", event.type, repr(event))
        if event.type == 'ESC':
            self.report({'INFO'}, "QuadWild-BiMDF retopo canceled")
            self.abort()
            return {'CANCELLED'}

        elif event.type == 'TIMER':
            if self.proc.poll() is None: # still running
                #print("poll None")
                self.progress.update("QuadWild-BiMDF working..." + str(self.prog))
                self.prog+=1
                return {'RUNNING_MODAL'}
            else:
                #print("poll not None")
                self.finished()
                return {'FINISHED'}
        return {'RUNNING_MODAL'}


    


class QuadWildPanel(bpy.types.Panel):
    """QuadWild Retopology (Bi-MDF version)"""     # Use this as a tooltip for menu items and buttons.
    bl_idname = "VIEW3D_PT_quadwild_bimdf"              # Unique identifier for buttons and menu items to reference.
    bl_label = "Retopologize with QuadWild-BiMDF"  # Display name in the interface.
    #bl_options = {'REGISTER', 'UNDO'}              # Enable undo for the operator.
    bl_space_type = 'VIEW_3D' # https://docs.blender.org/api/current/bpy_types_enum_items/space_type_items.html#rna-enum-space-type-items
    bl_region_type = 'UI' # https://docs.blender.org/api/current/bpy_types_enum_items/region_type_items.html#rna-enum-region-type-items
    bl_category = "Retopo"

    def draw(self, context):
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
        col.row().operator(QuadWildOperator.bl_idname)

#def menu_func(self, context):
#    self.layout.operator(QuadWildRetopology.bl_idname)

CLASSES = [
        QuadWildPropGroup,
        QuadWildOperator,
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
    print(__file__)
    register()
    #bpy.ops.quadwild_bimdf.op()

