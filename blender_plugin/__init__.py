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

class QuadWildPropGroup(bpy.types.PropertyGroup):
    scale:          bpy.props.FloatProperty(name='scale',               default=1)
    preprocess:     bpy.props.BoolProperty (name='Auto-preprocess',     default=True)
    sharp_thresh:   bpy.props.FloatProperty(name='Sharp edge thresh',   default=35/180*math.pi, min=0., max=math.pi, unit='ROTATION')
    alpha:          bpy.props.FloatProperty(name="alpha",               default=0.01, min=0., max=1.)
    align:          bpy.props.BoolProperty (name='Enable alignment',    default=False)
    align_weight:   bpy.props.FloatProperty(name='Alignment weight',    default=0.1, min=0., max=10.)
    create_new_obj: bpy.props.BoolProperty (name='Create new object',   default=True)

class QuadWildOperator(bpy.types.Operator):
    """QuadWild Retopology (Bi-MDF version)"""     # Use this as a tooltip for menu items and buttons.
    bl_idname = "quadwild_bimdf.op"                # Unique identifier for buttons and menu items to reference.
    bl_label = "Retopologize"                      # Display name in the interface.
    bl_options = {'REGISTER', 'UNDO'}              # Enable undo for the operator.

    def execute(self, context):
        obj = context.active_object
        pg = obj.quadwild_propgrp
        print(pg.alpha)
        return {'FINISHED'}


class QuadWildPanel(bpy.types.Panel):
    """QuadWild Retopology (Bi-MDF version)"""     # Use this as a tooltip for menu items and buttons.
    bl_idname = "VIEW3D_PT_quadwild_bimdf"              # Unique identifier for buttons and menu items to reference.
    bl_label = "Retopologize with QuadWild-BiMDF"  # Display name in the interface.
    #bl_options = {'REGISTER', 'UNDO'}              # Enable undo for the operator.
    bl_space_type = 'VIEW_3D' # https://docs.blender.org/api/current/bpy_types_enum_items/space_type_items.html#rna-enum-space-type-items
    bl_region_type = 'UI' # https://docs.blender.org/api/current/bpy_types_enum_items/region_type_items.html#rna-enum-region-type-items
    bl_category = "Retopo"

    def draw(self, context):
        col = self.layout.column()
        obj = context.active_object
        pg = obj.quadwild_propgrp

        box = col.box()
        box.label(text="QW Settings:")
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
        ]
def register():
    print("DEBUG: QuadWild-BiMDF register()")
    for cl in CLASSES:
        bpy.utils.register_class(cl)
    bpy.types.Object.quadwild_propgrp = bpy.props.PointerProperty(type=QuadWildPropGroup)
    #bpy.types.VIEW3D_MT_object.append(menu_func)  # Adds the new operator to an existing menu.

def unregister():
    for cl in CLASSES:
        bpy.utils.unregister_class(cl)
    del bpy.types.Object.quadwild_propgrp


if __name__ == "__main__":
    register()
