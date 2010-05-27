/*
 * mx-offscreen.h: An offscreen texture container
 *
 * Copyright 2010 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * Boston, MA 02111-1307, USA.
 *
 * Written by: Chris Lord <chris@linux.intel.com>
 *
 */

#include "mx-offscreen.h"
#include "mx-private.h"

static void clutter_container_iface_init (ClutterContainerIface *iface);

G_DEFINE_TYPE_WITH_CODE (MxOffscreen, mx_offscreen, CLUTTER_TYPE_TEXTURE,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_CONTAINER,
                                                clutter_container_iface_init))

#define OFFSCREEN_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MX_TYPE_OFFSCREEN, MxOffscreenPrivate))

struct _MxOffscreenPrivate
{
  guint         pick_child  : 1;
  guint         auto_update : 1;
  guint         redirect_enabled     : 1;

  guint         acc_enabled : 1;
  guint         blend_set   : 1;

  ClutterActor *child;

  CoglHandle    fbo;
  CoglHandle    acc_material;
  CoglHandle    acc_fbo;
};

enum
{
  PROP_0,

  PROP_CHILD,
  PROP_PICK_CHILD,
  PROP_AUTO_UPDATE,
  PROP_REDIRECT_ENABLED,
  PROP_BUFFER,
  PROP_ACC_ENABLED,
  PROP_ACC_MATERIAL
};


static void
mx_offscreen_add (ClutterContainer *container,
                  ClutterActor     *actor)
{
  mx_offscreen_set_child (MX_OFFSCREEN (container), actor);
}

static void
mx_offscreen_remove (ClutterContainer *container,
                     ClutterActor     *actor)
{
  MxOffscreen *self = MX_OFFSCREEN (container);

  if (self->priv->child == actor)
    mx_offscreen_set_child (self, NULL);
}

static void
mx_offscreen_foreach (ClutterContainer *container,
                      ClutterCallback   callback,
                      gpointer          user_data)
{
  MxOffscreenPrivate *priv = MX_OFFSCREEN (container)->priv;

  if (priv->child)
    callback (priv->child, user_data);
}

static void
clutter_container_iface_init (ClutterContainerIface *iface)
{
  iface->add = mx_offscreen_add;
  iface->remove = mx_offscreen_remove;
  iface->foreach = mx_offscreen_foreach;
}

static void
mx_offscreen_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  MxOffscreen *self = MX_OFFSCREEN (object);

  switch (property_id)
    {
    case PROP_CHILD:
      g_value_set_object (value, (GObject *)mx_offscreen_get_child (self));
      break;

    case PROP_PICK_CHILD:
      g_value_set_boolean (value, mx_offscreen_get_pick_child (self));
      break;

    case PROP_AUTO_UPDATE:
      g_value_set_boolean (value, mx_offscreen_get_auto_update (self));
      break;

    case PROP_REDIRECT_ENABLED:
      g_value_set_boolean (value, mx_offscreen_get_redirect_enabled (self));
      break;

    case PROP_BUFFER:
      g_value_set_pointer (value, mx_offscreen_get_buffer (self));
      break;

    case PROP_ACC_ENABLED:
      g_value_set_boolean (value, mx_offscreen_get_accumulation_enabled (self));
      break;

    case PROP_ACC_MATERIAL:
      g_value_set_pointer (value,
                           mx_offscreen_get_accumulation_material (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mx_offscreen_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  MxOffscreen *self = MX_OFFSCREEN (object);

  switch (property_id)
    {
    case PROP_CHILD:
      mx_offscreen_set_child (self, (ClutterActor *)g_value_get_object (value));
      break;

    case PROP_PICK_CHILD:
      mx_offscreen_set_pick_child (self, g_value_get_boolean (value));
      break;

    case PROP_AUTO_UPDATE:
      mx_offscreen_set_auto_update (self, g_value_get_boolean (value));
      break;

    case PROP_REDIRECT_ENABLED:
      mx_offscreen_set_redirect_enabled (self, g_value_get_boolean (value));
      break;

    case PROP_ACC_ENABLED:
      mx_offscreen_set_accumulation_enabled (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mx_offscreen_destroy (ClutterActor *actor)
{
  MxOffscreenPrivate *priv = MX_OFFSCREEN (actor)->priv;

  if (priv->child)
    {
      clutter_actor_destroy (priv->child);
      priv->child = NULL;
    }

  if (CLUTTER_ACTOR_CLASS (mx_offscreen_parent_class)->destroy)
    CLUTTER_ACTOR_CLASS (mx_offscreen_parent_class)->destroy (actor);
}

static void
mx_offscreen_dispose (GObject *object)
{
  MxOffscreenPrivate *priv = MX_OFFSCREEN (object)->priv;

  if (priv->fbo)
    {
      cogl_handle_unref (priv->fbo);
      priv->fbo = NULL;
    }

  if (priv->acc_material)
    {
      cogl_handle_unref (priv->acc_material);
      priv->acc_material = NULL;
    }

  if (priv->acc_fbo)
    {
      cogl_handle_unref (priv->acc_fbo);
      priv->acc_fbo = NULL;
    }

  G_OBJECT_CLASS (mx_offscreen_parent_class)->dispose (object);
}

static void
mx_offscreen_finalize (GObject *object)
{
  G_OBJECT_CLASS (mx_offscreen_parent_class)->finalize (object);
}

static void
mx_offscreen_get_preferred_width (ClutterActor *actor,
                                  gfloat        for_height,
                                  gfloat       *min_width_p,
                                  gfloat       *nat_width_p)
{
  MxOffscreenPrivate *priv = MX_OFFSCREEN (actor)->priv;

  if (priv->child)
    {
      clutter_actor_get_preferred_width (priv->child,
                                         for_height,
                                         min_width_p,
                                         nat_width_p);
    }
  else
    {
      if (min_width_p)
        *min_width_p = 0;
      if (nat_width_p)
        *nat_width_p = 0;
    }
}

static void
mx_offscreen_get_preferred_height (ClutterActor *actor,
                                   gfloat        for_width,
                                   gfloat       *min_height_p,
                                   gfloat       *nat_height_p)
{
  MxOffscreenPrivate *priv = MX_OFFSCREEN (actor)->priv;

  if (priv->child)
    {
      clutter_actor_get_preferred_height (priv->child,
                                          for_width,
                                          min_height_p,
                                          nat_height_p);
    }
  else
    {
      if (min_height_p)
        *min_height_p = 0;
      if (nat_height_p)
        *nat_height_p = 0;
    }
}

static void
mx_offscreen_allocate (ClutterActor           *actor,
                       const ClutterActorBox  *box,
                       ClutterAllocationFlags  flags)
{
  MxOffscreenPrivate *priv = MX_OFFSCREEN (actor)->priv;

  CLUTTER_ACTOR_CLASS (mx_offscreen_parent_class)->allocate (actor, box, flags);

  if (priv->child)
    {
      ClutterActorBox child_box;

      child_box.x1 = 0;
      child_box.y1 = 0;
      child_box.x2 = box->x2 - box->x1;
      child_box.y2 = box->y2 - box->y1;

      clutter_actor_allocate (priv->child, &child_box, flags);
    }
}

static void
mx_offscreen_toggle_shaders (MxOffscreen  *offscreen,
                             GList       **disabled_shaders,
                             gboolean      enable)
{
  ClutterActor *actor = (ClutterActor *)offscreen;

  if (enable)
    {
      GList *s;

      for (s = *disabled_shaders; s; s = s->next)
        clutter_shader_set_is_enabled ((ClutterShader *)s->data, TRUE);

      g_list_free (*disabled_shaders);
      *disabled_shaders = NULL;
    }
  else
    {
      *disabled_shaders = NULL;
      do
        {
          ClutterShader *shader = clutter_actor_get_shader (actor);
          if (shader && clutter_shader_get_is_enabled (shader))
            {
              clutter_shader_set_is_enabled (shader, FALSE);
              *disabled_shaders = g_list_prepend (*disabled_shaders, shader);
            }
        } while ((actor = clutter_actor_get_parent (actor)));
    }
}

static gboolean
mx_offscreen_ensure_buffers (MxOffscreen *offscreen)
{
#if CLUTTER_CHECK_VERSION(1,2,0)
  CoglHandle texture;
  gboolean sync_size;
  gfloat width, height;

  MxOffscreenPrivate *priv = offscreen->priv;

  clutter_actor_get_size (priv->child, &width, &height);
  sync_size = clutter_texture_get_sync_size (CLUTTER_TEXTURE (offscreen));

  /* Check our texture exists and is the correct size */
  texture = clutter_texture_get_cogl_texture (CLUTTER_TEXTURE (offscreen));
  if (!texture ||
      (sync_size && ((cogl_texture_get_width (texture) != (guint)width) ||
                     (cogl_texture_get_height (texture) != (guint)height))))
    {
      texture = cogl_texture_new_with_size ((guint)width,
                                            (guint)height,
                                            COGL_TEXTURE_NO_SLICING,
                                            COGL_PIXEL_FORMAT_RGBA_8888_PRE);
      clutter_texture_set_cogl_texture (CLUTTER_TEXTURE (offscreen), texture);
      if (texture)
        cogl_handle_unref (texture);
    }

  /* The notification of setting a texture will trigger a callback that
   * creates the fbo. Doing it this ways lets the texture be overriden
   * externally.
   */
  if (texture && priv->fbo)
    return TRUE;
  else
#endif
    return FALSE;
}

static CoglHandle
mx_offscreen_material_get_texture (CoglHandle material, gint layer_n)
{
  const GList *layers = cogl_material_get_layers (material);
  CoglHandle layer = g_list_nth_data ((GList *)layers, layer_n);
  return layer ? cogl_material_layer_get_texture (layer) : NULL;
}

static gboolean
mx_offscreen_ensure_accumulation_buffer (MxOffscreen *offscreen)
{
  guint width, height;
  CoglHandle texture, acc_texture;
  MxOffscreenPrivate *priv = offscreen->priv;

  texture = clutter_texture_get_cogl_texture (CLUTTER_TEXTURE (offscreen));
  if (!texture)
    return FALSE;

  acc_texture = mx_offscreen_material_get_texture (priv->acc_material, 0);

  width = cogl_texture_get_width (texture);
  height = cogl_texture_get_height (texture);

  if (!acc_texture ||
      (cogl_texture_get_width (acc_texture) != width) ||
      (cogl_texture_get_height (acc_texture) != height))
    {
      if (priv->acc_fbo)
        {
          cogl_handle_unref (priv->acc_fbo);
          priv->acc_fbo = NULL;
        }

      texture = cogl_texture_new_with_size (width,
                                            height,
                                            COGL_TEXTURE_NO_SLICING,
                                            COGL_PIXEL_FORMAT_RGBA_8888_PRE);
      cogl_material_set_layer (priv->acc_material, 0, texture);

      if (texture)
        {
          CoglColor color;

          priv->acc_fbo = cogl_offscreen_new_to_texture (texture);

          /* Clear the newly created texture */
          cogl_color_set_from_4ub (&color, 0, 0, 0, 0);
          cogl_push_framebuffer (priv->acc_fbo);
          cogl_clear (&color, COGL_BUFFER_BIT_COLOR);
          cogl_pop_framebuffer ();

          cogl_handle_unref (texture);

          return TRUE;
        }

      return FALSE;
    }

  return TRUE;
}

static void
mx_offscreen_paint (ClutterActor *actor)
{
  MxOffscreen *self = MX_OFFSCREEN (actor);
  MxOffscreenPrivate *priv = self->priv;

  if (!priv->child)
    return;

  if (!priv->redirect_enabled)
    {
      GList *disabled_shaders;

      /* Disable our shader when we paint with pass-through. */
      mx_offscreen_toggle_shaders (self, &disabled_shaders, FALSE);
      clutter_actor_paint (priv->child);
      mx_offscreen_toggle_shaders (self, &disabled_shaders, TRUE);
    }
  else
    {
      if (priv->auto_update)
        mx_offscreen_update (self);

      if (priv->acc_enabled && mx_offscreen_ensure_accumulation_buffer (self))
        {
          gfloat width, height;
          CoglColor zero_color;

          CoglHandle material =
            clutter_texture_get_cogl_material (CLUTTER_TEXTURE (actor));

          /* Blend the texture onto the accumulation buffer */
          cogl_push_framebuffer (priv->acc_fbo);
          cogl_color_set_from_4ub (&zero_color, 0, 0, 0, 0);
          cogl_clear (&zero_color,
                      COGL_BUFFER_BIT_STENCIL |
                      COGL_BUFFER_BIT_DEPTH);
          cogl_set_source (material);
          cogl_rectangle (-1, 1, 1, -1);
          cogl_pop_framebuffer ();

          /* Draw the accumulation buffer */
          clutter_actor_get_size (actor, &width, &height);
          cogl_set_source (priv->acc_material);
          cogl_rectangle (0, 0, width, height);
        }
      else
        CLUTTER_ACTOR_CLASS (mx_offscreen_parent_class)->paint (actor);
    }
}

static void
mx_offscreen_pick (ClutterActor *actor, const ClutterColor *color)
{
  MxOffscreenPrivate *priv = MX_OFFSCREEN (actor)->priv;

  if (priv->pick_child && priv->child)
    clutter_actor_paint (priv->child);
}

static void
mx_offscreen_map (ClutterActor *actor)
{
  MxOffscreenPrivate *priv = MX_OFFSCREEN (actor)->priv;

  CLUTTER_ACTOR_CLASS (mx_offscreen_parent_class)->map (actor);

  if (priv->child)
    clutter_actor_map (priv->child);
}

static void
mx_offscreen_unmap (ClutterActor *actor)
{
  MxOffscreenPrivate *priv = MX_OFFSCREEN (actor)->priv;

  if (priv->child)
    clutter_actor_unmap (priv->child);

  CLUTTER_ACTOR_CLASS (mx_offscreen_parent_class)->unmap (actor);
}

static void
mx_offscreen_real_paint_child (MxOffscreen *self)
{
  MxOffscreenPrivate *priv = self->priv;

  if (priv->child)
    clutter_actor_paint (priv->child);
}

static void
mx_offscreen_class_init (MxOffscreenClass *klass)
{
  GParamSpec *pspec;

  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MxOffscreenPrivate));

  object_class->get_property = mx_offscreen_get_property;
  object_class->set_property = mx_offscreen_set_property;
  object_class->dispose = mx_offscreen_dispose;
  object_class->finalize = mx_offscreen_finalize;

  actor_class->get_preferred_width = mx_offscreen_get_preferred_width;
  actor_class->get_preferred_height = mx_offscreen_get_preferred_height;
  actor_class->allocate = mx_offscreen_allocate;
  actor_class->paint = mx_offscreen_paint;
  actor_class->pick = mx_offscreen_pick;
  actor_class->map = mx_offscreen_map;
  actor_class->unmap = mx_offscreen_unmap;
  actor_class->destroy = mx_offscreen_destroy;

  klass->paint_child = mx_offscreen_real_paint_child;

  pspec = g_param_spec_object ("child",
                               "Child",
                               "Child actor of the offscreen texture.",
                               CLUTTER_TYPE_ACTOR,
                               MX_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_CHILD, pspec);

  pspec = g_param_spec_boolean ("pick-child",
                                "Pick child",
                                "Whether to pick the child.",
                                FALSE,
                                MX_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_PICK_CHILD, pspec);

  pspec = g_param_spec_boolean ("auto-update",
                                "Auto update",
                                "Update child actor automatically "
                                "when painting.",
                                TRUE,
                                MX_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_AUTO_UPDATE, pspec);

  pspec = g_param_spec_boolean ("redirect-enabled",
                                "Redirect Enabled",
                                "Enable redirection of the child actor to "
                                "the off-screen surface.",
                                TRUE,
                                MX_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_REDIRECT_ENABLED, pspec);

  pspec = g_param_spec_pointer ("buffer",
                                "Buffer",
                                "The off-screen buffer used to draw the child.",
                                MX_PARAM_READABLE);
  g_object_class_install_property (object_class, PROP_BUFFER, pspec);

  pspec = g_param_spec_boolean ("accumulation-enabled",
                                "Accumulation enabled",
                                "Enable an accumulation buffer via a "
                                "secondary buffer.",
                                FALSE,
                                MX_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_ACC_ENABLED, pspec);

  pspec = g_param_spec_pointer ("accumulation-material",
                                "Accumulation material",
                                "Material used for the accumulation buffer.",
                                MX_PARAM_READABLE);
  g_object_class_install_property (object_class, PROP_ACC_MATERIAL, pspec);
}

#if CLUTTER_CHECK_VERSION(1,2,0)
static void
mx_offscreen_cogl_texture_notify (MxOffscreen *self)
{
  CoglMatrix matrix;
  ClutterActor *stage;
  ClutterPerspective perspective;
  gfloat z_camera, width, height;

  MxOffscreenPrivate *priv = self->priv;
  CoglHandle texture =
    clutter_texture_get_cogl_texture (CLUTTER_TEXTURE (self));

  /* Recreated the texture, get rid of the fbo */
  if (priv->fbo)
    {
      cogl_handle_unref (priv->fbo);
      priv->fbo = NULL;
    }

  if (!texture)
    return;

  /* Create fbo */
  priv->fbo = cogl_offscreen_new_to_texture (texture);
  if (!priv->fbo)
    {
      g_warning (G_STRLOC ": Unable to create offscreen buffer for actor");
      return;
    }

  /* Setup the viewport (code derived from Clutter) */
  /* FIXME: This code will eventually be a public function in Clutter,
   *        so replace this when it is.
   */
  cogl_push_framebuffer (priv->fbo);

  stage = clutter_actor_get_stage (priv->child);
  clutter_stage_get_perspective (CLUTTER_STAGE (stage), &perspective);

  width = cogl_texture_get_width (texture);
  height = cogl_texture_get_height (texture);

  cogl_set_viewport (0, 0, width, height);
  cogl_perspective (perspective.fovy,
                    perspective.aspect,
                    perspective.z_near,
                    perspective.z_far);

  cogl_get_projection_matrix (&matrix);
  z_camera = 0.5 * matrix.xx;

  cogl_matrix_init_identity (&matrix);
  cogl_matrix_translate (&matrix, -0.5f, -0.5f, -z_camera);
  cogl_matrix_scale (&matrix, 1.f / width, -1.f / height, 1.f / width);
  cogl_matrix_translate (&matrix, 0.f, -1.f * height, 0.f);

  cogl_set_modelview_matrix (&matrix);

  cogl_pop_framebuffer ();

  g_object_notify (G_OBJECT (self), "buffer");
}
#endif

static void
mx_offscreen_init (MxOffscreen *self)
{
  MxOffscreenPrivate *priv = self->priv = OFFSCREEN_PRIVATE (self);

  priv->auto_update = TRUE;
  priv->redirect_enabled = TRUE;

#if CLUTTER_CHECK_VERSION(1,2,0)
  g_signal_connect (self, "notify::cogl-texture",
                    G_CALLBACK (mx_offscreen_cogl_texture_notify), NULL);
#endif
}

ClutterActor *
mx_offscreen_new (void)
{
  return g_object_new (MX_TYPE_OFFSCREEN, NULL);
}

void
mx_offscreen_set_child (MxOffscreen *offscreen, ClutterActor *actor)
{
  MxOffscreenPrivate *priv;

  g_return_if_fail (MX_IS_OFFSCREEN (offscreen));

  priv = offscreen->priv;
  if (priv->child == actor)
    return;

  if (priv->child)
    {
      ClutterActor *old_child = g_object_ref (priv->child);

      clutter_actor_unparent (priv->child);
      priv->child = NULL;

      g_signal_emit_by_name (offscreen, "actor-removed", old_child);

      g_object_unref (old_child);
    }

  if (actor)
    {
      priv->child = actor;
      clutter_actor_set_parent (actor, CLUTTER_ACTOR (offscreen));

      g_signal_emit_by_name (offscreen, "actor-added", actor);
    }

  clutter_actor_queue_relayout (CLUTTER_ACTOR (offscreen));

  g_object_notify (G_OBJECT (offscreen), "child");
}

ClutterActor *
mx_offscreen_get_child (MxOffscreen *offscreen)
{
  g_return_val_if_fail (MX_IS_OFFSCREEN (offscreen), NULL);
  return offscreen->priv->child;
}

void
mx_offscreen_set_pick_child (MxOffscreen *offscreen, gboolean pick)
{
  g_return_if_fail (MX_IS_OFFSCREEN (offscreen));

  if (offscreen->priv->pick_child != pick)
    {
      offscreen->priv->pick_child = pick;
      g_object_notify (G_OBJECT (offscreen), "pick-child");
    }
}

gboolean
mx_offscreen_get_pick_child (MxOffscreen *offscreen)
{
  g_return_val_if_fail (MX_IS_OFFSCREEN (offscreen), FALSE);
  return offscreen->priv->pick_child;
}

void
mx_offscreen_set_auto_update (MxOffscreen *offscreen, gboolean auto_update)
{
  MxOffscreenPrivate *priv;

  g_return_if_fail (MX_IS_OFFSCREEN (offscreen));

  priv = offscreen->priv;

  if (priv->auto_update != auto_update)
    {
      priv->auto_update = auto_update;
      g_object_notify (G_OBJECT (offscreen), "auto-update");
    }
}

gboolean
mx_offscreen_get_auto_update (MxOffscreen *offscreen)
{
  g_return_val_if_fail (MX_IS_OFFSCREEN (offscreen), FALSE);
  return offscreen->priv->auto_update;
}

void
mx_offscreen_update (MxOffscreen *offscreen)
{
#if CLUTTER_CHECK_VERSION(1,2,0)
  CoglColor zero_colour;
  GList *disabled_shaders;

  MxOffscreenPrivate *priv = offscreen->priv;

  if (!mx_offscreen_ensure_buffers (offscreen))
    {
      g_warning (G_STRLOC ": Unable to create necessary buffers");
      return;
    }

  /* Disable shaders when we paint our off-screen children */
  mx_offscreen_toggle_shaders (offscreen, &disabled_shaders, FALSE);

  /* Start drawing */
  cogl_push_framebuffer (priv->fbo);
  cogl_push_matrix ();

  /* Clear */
  cogl_color_set_from_4ub (&zero_colour, 0x00, 0x00, 0x00, 0x00);
  cogl_clear (&zero_colour,
              COGL_BUFFER_BIT_COLOR |
              COGL_BUFFER_BIT_STENCIL |
              COGL_BUFFER_BIT_DEPTH);

  /* Draw actor */
  MX_OFFSCREEN_GET_CLASS (offscreen)->paint_child (offscreen);

  /* Restore state */
  cogl_pop_matrix ();
  cogl_pop_framebuffer ();

  /* Re-enable shaders */
  mx_offscreen_toggle_shaders (offscreen, &disabled_shaders, TRUE);

#else
  static gboolean warned = FALSE;
  if (warned)
    {
      g_warning ("Clutter 1.2.0 is required for the offscreen actor. Consider "
                 "using clutter_texture_new_from_actor() instead.");
      warned = TRUE;
    }
#endif
}

void
mx_offscreen_set_redirect_enabled (MxOffscreen *offscreen,
                                   gboolean     enabled)
{
  MxOffscreenPrivate *priv;

  g_return_if_fail (MX_IS_OFFSCREEN (offscreen));

  priv = offscreen->priv;
  if (priv->redirect_enabled != enabled)
    {
      priv->redirect_enabled = enabled;

      if (enabled && priv->acc_fbo)
        {
          CoglColor color;

          /* Clear the accumulation buffer when the offscreen is
           * enabled. As the child has been drawn without updating,
           * the contents of the accumulation buffer is invalid.
           */
          cogl_color_set_from_4ub (&color, 0, 0, 0, 0);
          cogl_push_framebuffer (priv->fbo);
          cogl_clear (&color, COGL_BUFFER_BIT_COLOR);
          cogl_pop_framebuffer ();
        }

      g_object_notify (G_OBJECT (offscreen), "enabled");

      clutter_actor_queue_redraw (CLUTTER_ACTOR (offscreen));
    }
}

gboolean
mx_offscreen_get_redirect_enabled (MxOffscreen *offscreen)
{
  g_return_val_if_fail (MX_IS_OFFSCREEN (offscreen), FALSE);
  return offscreen->priv->redirect_enabled;
}

CoglHandle
mx_offscreen_get_buffer (MxOffscreen *offscreen)
{
  g_return_val_if_fail (MX_IS_OFFSCREEN (offscreen), NULL);
  mx_offscreen_ensure_buffers (offscreen);
  return offscreen->priv->fbo;
}

void
mx_offscreen_set_accumulation_enabled (MxOffscreen *offscreen,
                                       gboolean     enable)
{
  MxOffscreenPrivate *priv;

  g_return_if_fail (MX_IS_OFFSCREEN (offscreen));

  priv = offscreen->priv;
  if (priv->acc_enabled != enable)
    {
      CoglHandle material =
        clutter_texture_get_cogl_material (CLUTTER_TEXTURE (offscreen));

      priv->acc_enabled = enable;

      if (enable)
        {
          CoglColor blend_color;

          GError *error = NULL;

          priv->acc_material = cogl_material_new ();

          /* Set the default blend string/level for accumulation */
          cogl_color_set_from_4ub (&blend_color, 128, 128, 128, 128);
          cogl_material_set_blend_constant (material, &blend_color);

          if (!cogl_material_set_blend (material,
                                        "RGBA=ADD(SRC_COLOR*(CONSTANT[A]),"
                                                 "DST_COLOR*(1-CONSTANT[A]))",
                                        &error))
            {
              g_warning (G_STRLOC ": Error setting blend string: %s",
                         error->message);
              g_error_free (error);
            }
        }
      else
        {
          cogl_handle_unref (priv->acc_material);
          priv->acc_material = NULL;

          if (priv->acc_fbo)
            {
              cogl_handle_unref (priv->acc_fbo);
              priv->acc_fbo = NULL;
            }

          /* Reset to the default blend string */
          cogl_material_set_blend (material,
                                   "RGBA=ADD(SRC_COLOR,"
                                   "DST_COLOR*(1-SRC_COLOR[A]))",
                                   NULL);
        }

      g_object_notify (G_OBJECT (offscreen), "accumulation-enabled");
    }
}

gboolean
mx_offscreen_get_accumulation_enabled (MxOffscreen *offscreen)
{
  g_return_val_if_fail (MX_IS_OFFSCREEN (offscreen), FALSE);
  return offscreen->priv->acc_enabled;
}

CoglHandle
mx_offscreen_get_accumulation_material (MxOffscreen *offscreen)
{
  g_return_val_if_fail (MX_IS_OFFSCREEN (offscreen), NULL);
  return offscreen->priv->acc_material;
}
