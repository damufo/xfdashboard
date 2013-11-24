/*
 * textbox: An actor representing an editable or read-only text-box
 *          with icons optinally
 * 
 * Copyright 2012-2013 Stephan Haller <nomad@froevel.de>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "textbox.h"
#include "enums.h"

#include <glib/gi18n-lib.h>
#include <math.h>

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardTextBox,
				xfdashboard_text_box,
				XFDASHBOARD_TYPE_BACKGROUND)
                                                
/* Private structure - access only by public API if needed */
#define XFDASHBOARD_TEXT_BOX_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_TEXT_BOX, XfdashboardTextBoxPrivate))

struct _XfdashboardTextBoxPrivate
{
	/* Actors for text box, hint label and icons */
	ClutterActor			*actorTextBox;
	ClutterActor			*actorHintLabel;
	ClutterActor			*actorPrimaryIcon;
	ClutterActor			*actorSecondaryIcon;

	/* Actor actions */
	ClutterAction			*primaryIconClickAction;
	ClutterAction			*secondaryIconClickAction;

	/* Settings */
	gfloat					padding;
	gfloat					spacing;

	gchar					*primaryIconName;
	gchar					*secondaryIconName;

	gchar					*textFont;
	ClutterColor			*textColor;

	gchar					*hintTextFont;
	ClutterColor			*hintTextColor;

	/* Internal variables */
	gboolean				showPrimaryIcon;
	gboolean				showSecondaryIcon;
};

/* Properties */
enum
{
	PROP_0,

	PROP_PADDING,
	PROP_SPACING,

	PROP_PRIMARY_ICON_NAME,
	PROP_SECONDARY_ICON_NAME,

	PROP_TEXT,
	PROP_TEXT_FONT,
	PROP_TEXT_COLOR,
	// TODO: PROP_SELECTION_COLOR,

	PROP_HINT_TEXT,
	PROP_HINT_TEXT_FONT,
	PROP_HINT_TEXT_COLOR,

	PROP_LAST
};

static GParamSpec* XfdashboardTextBoxProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_TEXT_CHANGED,

	SIGNAL_PRIMARY_ICON_CLICKED,
	SIGNAL_SECONDARY_ICON_CLICKED,

	SIGNAL_LAST
};

static guint XfdashboardTextBoxSignals[SIGNAL_LAST]={ 0, };

/* Private constants */
#define DEFAULT_ICON_SIZE	16
#define DEFAULT_PADDING		4.0f
#define DEFAULT_SPACING		4.0f

static ClutterColor		defaultTextColor={ 0xff, 0xff, 0xff, 0xff };
static ClutterColor		defaultHintTextColor={ 0xc0, 0xc0, 0xc0, 0xff };

/* IMPLEMENTATION: Private variables and methods */

/* Text of editable text box has changed */
static void _xfdashboard_text_box_on_text_changed(XfdashboardTextBox *self, gpointer inUserData)
{
	XfdashboardTextBoxPrivate	*priv;
	ClutterText					*actorText;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));
	g_return_if_fail(CLUTTER_IS_TEXT(inUserData));

	priv=self->priv;
	actorText=CLUTTER_TEXT(inUserData);

	/* Show hint label depending if text box is empty or not */
	if(xfdashboard_text_box_is_empty(self))
	{
		clutter_actor_show(priv->actorHintLabel);
	}
		else
		{
			clutter_actor_hide(priv->actorHintLabel);
		}

	/* Emit signal for text changed */
	g_signal_emit(self, XfdashboardTextBoxSignals[SIGNAL_TEXT_CHANGED], 0, clutter_text_get_text(actorText));
}

/* Primary icon was clicked */
static void _xfdashboard_text_box_on_primary_icon_clicked(ClutterClickAction *inAction,
															ClutterActor *inActor,
															gpointer inUserData)
{
	XfdashboardTextBox			*self;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(inUserData));

	self=XFDASHBOARD_TEXT_BOX(inUserData);

	/* Emit signal for clicking primary icon */
	g_signal_emit(self, XfdashboardTextBoxSignals[SIGNAL_PRIMARY_ICON_CLICKED], 0);
}

/* Secondary icon was clicked */
static void _xfdashboard_text_box_on_secondary_icon_clicked(ClutterClickAction *inAction,
															ClutterActor *inActor,
															gpointer inUserData)
{
	XfdashboardTextBox			*self;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(inUserData));

	self=XFDASHBOARD_TEXT_BOX(inUserData);

	/* Emit signal for clicking secondary icon */
	g_signal_emit(self, XfdashboardTextBoxSignals[SIGNAL_SECONDARY_ICON_CLICKED], 0);
}

/* IMPLEMENTATION: ClutterActor */

/* Actor got key focus */
static void _xfdashboard_text_box_key_focus_in(ClutterActor *inActor)
{
	XfdashboardTextBox			*self=XFDASHBOARD_TEXT_BOX(inActor);
	XfdashboardTextBoxPrivate	*priv=self->priv;
	ClutterStage				*stage;

	/* Push key focus forward to text box */
	stage=CLUTTER_STAGE(clutter_actor_get_stage(inActor));
	clutter_stage_set_key_focus(stage, priv->actorTextBox);
}

/* Show all children of this one */
static void _xfdashboard_text_box_show(ClutterActor *inActor)
{
	XfdashboardTextBox			*self=XFDASHBOARD_TEXT_BOX(inActor);
	XfdashboardTextBoxPrivate	*priv=self->priv;

	/* Show icons */
	if(priv->showPrimaryIcon!=FALSE)
	{
		clutter_actor_show(CLUTTER_ACTOR(priv->actorPrimaryIcon));
	}

	if(priv->showSecondaryIcon!=FALSE)
	{
		clutter_actor_show(CLUTTER_ACTOR(priv->actorSecondaryIcon));
	}

	/* Show hint label depending if text box is empty or not */
	if(xfdashboard_text_box_is_empty(self))
	{
		clutter_actor_show(priv->actorHintLabel);
	}
		else
		{
			clutter_actor_hide(priv->actorHintLabel);
		}

	clutter_actor_show(CLUTTER_ACTOR(self));
}

/* Get preferred width/height */
static void _xfdashboard_text_box_get_preferred_height(ClutterActor *self,
														gfloat inForWidth,
														gfloat *outMinHeight,
														gfloat *outNaturalHeight)
{
	XfdashboardTextBoxPrivate	*priv=XFDASHBOARD_TEXT_BOX(self)->priv;
	gfloat						minHeight, naturalHeight;
	gfloat						childMinHeight, childNaturalHeight;
	
	minHeight=naturalHeight=0.0f;

	/* Regardless if text or hint label is visible or not. Both actors' height
	 * will be requested and the larger one used */
	clutter_actor_get_preferred_height(CLUTTER_ACTOR(priv->actorTextBox),
										inForWidth,
										&minHeight, &naturalHeight);

	clutter_actor_get_preferred_height(CLUTTER_ACTOR(priv->actorHintLabel),
										inForWidth,
										&childMinHeight, &childNaturalHeight);

	if(childMinHeight>minHeight) minHeight=childMinHeight;
	if(childNaturalHeight>naturalHeight) naturalHeight=childNaturalHeight;

	// Add padding
	minHeight+=2*priv->padding;
	naturalHeight+=2*priv->padding;

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

static void _xfdashboard_text_box_get_preferred_width(ClutterActor *self,
														gfloat inForHeight,
														gfloat *outMinWidth,
														gfloat *outNaturalWidth)
{
	XfdashboardTextBoxPrivate	*priv=XFDASHBOARD_TEXT_BOX(self)->priv;
	gfloat						minWidth, naturalWidth;
	gfloat						childMinWidth, childNaturalWidth;
	gint						numberChildren=0;

	minWidth=naturalWidth=0.0f;

	/* Determine size of primary icon if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorPrimaryIcon))
	{
		clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorPrimaryIcon),
											inForHeight,
											&childMinWidth, &childNaturalWidth);

		minWidth+=childMinWidth;
		naturalWidth+=childNaturalWidth;
		numberChildren++;
	}

	/* Determine size of editable text box if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorTextBox))
	{
		clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorTextBox),
											inForHeight,
											&childMinWidth, &childNaturalWidth);

		minWidth+=childMinWidth;
		naturalWidth+=childNaturalWidth;
		numberChildren++;
	}

	/* Determine size of hint label if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorHintLabel))
	{
		clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorHintLabel),
											inForHeight,
											&childMinWidth, &childNaturalWidth);

		minWidth+=childMinWidth;
		naturalWidth+=childNaturalWidth;
		numberChildren++;
	}

	/* Determine size of secondary icon if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorSecondaryIcon))
	{
		clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorSecondaryIcon),
											inForHeight,
											&childMinWidth, &childNaturalWidth);

		minWidth+=childMinWidth;
		naturalWidth+=childNaturalWidth;
	}

	/* Add spacing for each child except the last one */
	if(numberChildren>1)
	{
		numberChildren--;
		minWidth+=(numberChildren*priv->spacing);
		naturalWidth+=(numberChildren*priv->spacing);
	}

	// Add padding
	minWidth+=2*priv->padding;
	naturalWidth+=2*priv->padding;

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* Allocate position and size of actor and its children*/
static void _xfdashboard_text_box_allocate(ClutterActor *self,
											const ClutterActorBox *inBox,
											ClutterAllocationFlags inFlags)
{
	XfdashboardTextBoxPrivate	*priv=XFDASHBOARD_TEXT_BOX(self)->priv;
	ClutterActorBox				*box=NULL;
	gfloat						left, right, top, bottom;
	gfloat						iconWidth, iconHeight;

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(xfdashboard_text_box_parent_class)->allocate(self, inBox, inFlags);

	/* Initialize bounding box (except right side) of allocation used in actors */
	left=top=priv->padding;
	right=clutter_actor_box_get_width(inBox)-priv->padding;
	bottom=clutter_actor_box_get_height(inBox)-priv->padding;

	/* Set allocation of primary icon if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorPrimaryIcon))
	{
		gfloat					childRight;
		ClutterContent			*iconContent;

		/* Get scale size of primary icon */
		iconContent=clutter_actor_get_content(priv->actorPrimaryIcon);
		clutter_content_get_preferred_size(iconContent, &iconWidth, &iconHeight);
		iconWidth=(bottom-top)*(iconWidth/iconHeight);

		/* Set allocation */
		childRight=left+iconWidth;

		box=clutter_actor_box_new(floor(left), floor(top), floor(childRight), floor(bottom));
		clutter_actor_allocate(CLUTTER_ACTOR(priv->actorPrimaryIcon), box, inFlags);
		clutter_actor_box_free(box);

		/* Adjust bounding box for next actor */
		left=childRight+priv->spacing;
	}

	/* Set allocation of secondary icon if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorSecondaryIcon))
	{
		gfloat					childLeft;
		ClutterContent			*iconContent;

		/* Get scale size of primary icon */
		iconContent=clutter_actor_get_content(priv->actorSecondaryIcon);
		clutter_content_get_preferred_size(iconContent, &iconWidth, &iconHeight);
		iconWidth=(bottom-top)*(iconWidth/iconHeight);

		/* Set allocation */
		childLeft=right-iconWidth;

		box=clutter_actor_box_new(floor(childLeft), floor(top), floor(right), floor(bottom));
		clutter_actor_allocate(CLUTTER_ACTOR(priv->actorSecondaryIcon), box, inFlags);
		clutter_actor_box_free(box);

		/* Adjust bounding box for next actor */
		right=childLeft-priv->spacing;
	}

	/* Set allocation of editable text box if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorTextBox))
	{
		gfloat					textHeight;

		/* Get height of text */
		clutter_actor_get_preferred_size(CLUTTER_ACTOR(priv->actorTextBox),
											NULL, NULL,
											NULL, &textHeight);

		/* Set allocation */
		box=clutter_actor_box_new(floor(left), floor(bottom-textHeight), floor(right), floor(bottom));
		clutter_actor_allocate(CLUTTER_ACTOR(priv->actorTextBox), box, inFlags);
		clutter_actor_box_free(box);
	}

	/* Set allocation of hint label if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorHintLabel))
	{
		gfloat					textHeight;

		/* Get height of label */
		clutter_actor_get_preferred_size(CLUTTER_ACTOR(priv->actorHintLabel),
											NULL, NULL,
											NULL, &textHeight);

		/* Set allocation */
		box=clutter_actor_box_new(floor(left), floor(bottom-textHeight), floor(right), floor(bottom));
		clutter_actor_allocate(CLUTTER_ACTOR(priv->actorHintLabel), box, inFlags);
		clutter_actor_box_free(box);
	}
}

/* Destroy this actor */
static void _xfdashboard_text_box_destroy(ClutterActor *self)
{
	/* Destroy each child actor when this actor is destroyed */
	XfdashboardTextBoxPrivate	*priv=XFDASHBOARD_TEXT_BOX(self)->priv;

	if(priv->actorTextBox)
	{
		clutter_actor_destroy(CLUTTER_ACTOR(priv->actorTextBox));
		priv->actorTextBox=NULL;
	}

	if(priv->actorHintLabel)
	{
		clutter_actor_destroy(CLUTTER_ACTOR(priv->actorHintLabel));
		priv->actorHintLabel=NULL;
	}

	if(priv->actorPrimaryIcon)
	{
		clutter_actor_destroy(CLUTTER_ACTOR(priv->actorPrimaryIcon));
		priv->actorPrimaryIcon=NULL;
	}

	if(priv->actorSecondaryIcon)
	{
		clutter_actor_destroy(CLUTTER_ACTOR(priv->actorSecondaryIcon));
		priv->actorSecondaryIcon=NULL;
	}

	/* Call parent's class destroy method */
	CLUTTER_ACTOR_CLASS(xfdashboard_text_box_parent_class)->destroy(self);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_text_box_dispose(GObject *inObject)
{
	/* Release our allocated variables */
	XfdashboardTextBoxPrivate	*priv=XFDASHBOARD_TEXT_BOX(inObject)->priv;

	if(priv->primaryIconName)
	{
		g_free(priv->primaryIconName);
		priv->primaryIconName=NULL;
	}

	if(priv->secondaryIconName)
	{
		g_free(priv->secondaryIconName);
		priv->secondaryIconName=NULL;
	}

	if(priv->textFont)
	{
		g_free(priv->textFont);
		priv->textFont=NULL;
	}

	if(priv->textColor)
	{
		clutter_color_free(priv->textColor);
		priv->textColor=NULL;
	}

	if(priv->hintTextFont)
	{
		g_free(priv->hintTextFont);
		priv->hintTextFont=NULL;
	}

	if(priv->hintTextColor)
	{
		clutter_color_free(priv->hintTextColor);
		priv->hintTextColor=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_text_box_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_text_box_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardTextBox			*self=XFDASHBOARD_TEXT_BOX(inObject);

	switch(inPropID)
	{
		case PROP_PADDING:
			xfdashboard_text_box_set_padding(self, g_value_get_float(inValue));
			break;

		case PROP_SPACING:
			xfdashboard_text_box_set_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_PRIMARY_ICON_NAME:
			xfdashboard_text_box_set_primary_icon(self, g_value_get_string(inValue));
			break;

		case PROP_SECONDARY_ICON_NAME:
			xfdashboard_text_box_set_secondary_icon(self, g_value_get_string(inValue));
			break;

		case PROP_TEXT:
			xfdashboard_text_box_set_text(self, g_value_get_string(inValue));
			break;

		case PROP_TEXT_FONT:
			xfdashboard_text_box_set_text_font(self, g_value_get_string(inValue));
			break;

		case PROP_TEXT_COLOR:
			xfdashboard_text_box_set_text_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_HINT_TEXT:
			xfdashboard_text_box_set_hint_text(self, g_value_get_string(inValue));
			break;

		case PROP_HINT_TEXT_FONT:
			xfdashboard_text_box_set_hint_text_font(self, g_value_get_string(inValue));
			break;

		case PROP_HINT_TEXT_COLOR:
			xfdashboard_text_box_set_hint_text_color(self, clutter_value_get_color(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_text_box_get_property(GObject *inObject,
												guint inPropID,
												GValue *outValue,
												GParamSpec *inSpec)
{
	XfdashboardTextBox				*self=XFDASHBOARD_TEXT_BOX(inObject);
	XfdashboardTextBoxPrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_PADDING:
			g_value_set_float(outValue, priv->padding);
			break;

		case PROP_SPACING:
			g_value_set_float(outValue, priv->spacing);
			break;

		case PROP_PRIMARY_ICON_NAME:
			g_value_set_string(outValue, priv->primaryIconName);
			break;

		case PROP_SECONDARY_ICON_NAME:
			g_value_set_string(outValue, priv->secondaryIconName);
			break;

		case PROP_TEXT:
			g_value_set_string(outValue, clutter_text_get_text(CLUTTER_TEXT(priv->actorTextBox)));
			break;

		case PROP_TEXT_FONT:
			g_value_set_string(outValue, priv->textFont);
			break;

		case PROP_TEXT_COLOR:
			clutter_value_set_color(outValue, priv->textColor);
			break;

		case PROP_HINT_TEXT:
			g_value_set_string(outValue, clutter_text_get_text(CLUTTER_TEXT(priv->actorHintLabel)));
			break;

		case PROP_HINT_TEXT_FONT:
			g_value_set_string(outValue, priv->hintTextFont);
			break;

		case PROP_HINT_TEXT_COLOR:
			clutter_value_set_color(outValue, priv->hintTextColor);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_text_box_class_init(XfdashboardTextBoxClass *klass)
{
	ClutterActorClass	*actorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_text_box_dispose;
	gobjectClass->set_property=_xfdashboard_text_box_set_property;
	gobjectClass->get_property=_xfdashboard_text_box_get_property;

	actorClass->show_all=_xfdashboard_text_box_show;
	actorClass->get_preferred_width=_xfdashboard_text_box_get_preferred_width;
	actorClass->get_preferred_height=_xfdashboard_text_box_get_preferred_height;
	actorClass->allocate=_xfdashboard_text_box_allocate;
	actorClass->destroy=_xfdashboard_text_box_destroy;
	actorClass->key_focus_in=_xfdashboard_text_box_key_focus_in;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardTextBoxPrivate));

	/* Define properties */
	XfdashboardTextBoxProperties[PROP_PADDING]=
		g_param_spec_float("padding",
							_("Padding"),
							_("Padding between background and elements"),
							0.0f, G_MAXFLOAT,
							DEFAULT_PADDING,
							G_PARAM_READWRITE);

	XfdashboardTextBoxProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
							_("Spacing"),
							_("Spacing between text and icon"),
							0.0f, G_MAXFLOAT,
							DEFAULT_SPACING,
							G_PARAM_READWRITE);

	XfdashboardTextBoxProperties[PROP_PRIMARY_ICON_NAME]=
		g_param_spec_string("primary-icon-name",
							_("Primary icon name"),
							_("Themed icon name or file name of primary icon shown left of text box"),
							NULL,
							G_PARAM_READWRITE);

	XfdashboardTextBoxProperties[PROP_SECONDARY_ICON_NAME]=
		g_param_spec_string("secondary-icon-name",
							_("Secondary icon name"),
							_("Themed icon name or file name of secondary icon shown right of text box"),
							NULL,
							G_PARAM_READWRITE);

	XfdashboardTextBoxProperties[PROP_TEXT]=
		g_param_spec_string("text",
							_("Text"),
							_("Text of editable text box"),
							N_(""),
							G_PARAM_READWRITE);

	XfdashboardTextBoxProperties[PROP_TEXT_FONT]=
		g_param_spec_string("text-font",
							_("Text font"),
							_("Font of editable text box"),
							NULL,
							G_PARAM_READWRITE);

	XfdashboardTextBoxProperties[PROP_TEXT_COLOR]=
		clutter_param_spec_color("text-color",
									_("Text color"),
									_("Color of text in editable text box"),
									&defaultTextColor,
									G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardTextBoxProperties[PROP_HINT_TEXT]=
		g_param_spec_string("hint-text",
							_("Hint text"),
							_("Hint text shown if editable text box is empty"),
							NULL,
							G_PARAM_READWRITE);

	XfdashboardTextBoxProperties[PROP_HINT_TEXT_FONT]=
		g_param_spec_string("hint-text-font",
							_("Hint text font"),
							_("Font of hint text shown if editable text box is empty"),
							NULL,
							G_PARAM_READWRITE);

	XfdashboardTextBoxProperties[PROP_HINT_TEXT_COLOR]=
		clutter_param_spec_color("hint-text-color",
									_("Hint text color"),
									_("Color of hint text shown if editable text box is empty"),
									&defaultHintTextColor,
									G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardTextBoxProperties);

	/* Define signals */
	XfdashboardTextBoxSignals[SIGNAL_TEXT_CHANGED]=
		g_signal_new("text-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardTextBoxClass, text_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__STRING,
						G_TYPE_NONE,
						1,
						G_TYPE_STRING);

	XfdashboardTextBoxSignals[SIGNAL_PRIMARY_ICON_CLICKED]=
		g_signal_new("primary-icon-clicked",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardTextBoxClass, primary_icon_clicked),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardTextBoxSignals[SIGNAL_SECONDARY_ICON_CLICKED]=
		g_signal_new("secondary-icon-clicked",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardTextBoxClass, secondary_icon_clicked),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_text_box_init(XfdashboardTextBox *self)
{
	XfdashboardTextBoxPrivate	*priv;

	priv=self->priv=XFDASHBOARD_TEXT_BOX_GET_PRIVATE(self);

	/* This actor is react on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set up default values */
	priv->padding=DEFAULT_PADDING;
	priv->spacing=DEFAULT_SPACING;
	priv->primaryIconName=NULL;
	priv->secondaryIconName=NULL;
	priv->textFont=NULL;
	priv->textColor=NULL;
	priv->hintTextFont=NULL;
	priv->hintTextColor=NULL;
	priv->showPrimaryIcon=FALSE;
	priv->showSecondaryIcon=FALSE;

	/* Create actors */
	priv->actorPrimaryIcon=clutter_actor_new();
	clutter_actor_hide(priv->actorPrimaryIcon);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->actorPrimaryIcon);
	clutter_actor_set_reactive(priv->actorPrimaryIcon, TRUE);

	priv->primaryIconClickAction=clutter_click_action_new();
	clutter_actor_add_action(priv->actorPrimaryIcon, priv->primaryIconClickAction);
	g_signal_connect(priv->primaryIconClickAction, "clicked", G_CALLBACK(_xfdashboard_text_box_on_primary_icon_clicked), self);

	priv->actorSecondaryIcon=clutter_actor_new();
	clutter_actor_hide(priv->actorSecondaryIcon);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->actorSecondaryIcon);
	clutter_actor_set_reactive(priv->actorSecondaryIcon, TRUE);

	priv->secondaryIconClickAction=clutter_click_action_new();
	clutter_actor_add_action(priv->actorSecondaryIcon, priv->secondaryIconClickAction);
	g_signal_connect(priv->secondaryIconClickAction, "clicked", G_CALLBACK(_xfdashboard_text_box_on_secondary_icon_clicked), self);

	priv->actorTextBox=clutter_text_new();
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->actorTextBox);
	clutter_actor_set_reactive(priv->actorTextBox, TRUE);
	clutter_text_set_selectable(CLUTTER_TEXT(priv->actorTextBox), TRUE);
	clutter_text_set_editable(CLUTTER_TEXT(priv->actorTextBox), TRUE);
	clutter_text_set_single_line_mode(CLUTTER_TEXT(priv->actorTextBox), TRUE);
	g_signal_connect_swapped(priv->actorTextBox, "text-changed", G_CALLBACK(_xfdashboard_text_box_on_text_changed), self);

	priv->actorHintLabel=clutter_text_new();
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->actorHintLabel);
	clutter_actor_set_reactive(priv->actorHintLabel, FALSE);
	clutter_text_set_selectable(CLUTTER_TEXT(priv->actorHintLabel), FALSE);
	clutter_text_set_editable(CLUTTER_TEXT(priv->actorHintLabel), FALSE);
	clutter_text_set_single_line_mode(CLUTTER_TEXT(priv->actorHintLabel), TRUE);
}

/* Implementation: Public API */

/* Create new actor */
ClutterActor* xfdashboard_text_box_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_TEXT_BOX, NULL));
}

/* Get/set padding of background to text and icon actors */
gfloat xfdashboard_text_box_get_padding(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), 0);

	return(self->priv->padding);
}

void xfdashboard_text_box_set_padding(XfdashboardTextBox *self, gfloat inPadding)
{
	XfdashboardTextBoxPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));
	g_return_if_fail(inPadding>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->padding!=inPadding)
	{
		priv->padding=inPadding;
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_PADDING]);
	}
}

/* Get/set spacing between text and icon actors */
gfloat xfdashboard_text_box_get_spacing(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), 0);

	return(self->priv->spacing);
}

void xfdashboard_text_box_set_spacing(XfdashboardTextBox *self, gfloat inSpacing)
{
	XfdashboardTextBoxPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));
	g_return_if_fail(inSpacing>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->spacing!=inSpacing)
	{
		priv->spacing=inSpacing;
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_SPACING]);
	}
}

/* Get/set text of editable text box */
gboolean xfdashboard_text_box_is_empty(XfdashboardTextBox *self)
{
	const gchar					*text;

	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), TRUE);

	text=clutter_text_get_text(CLUTTER_TEXT(self->priv->actorTextBox));

	return(text==NULL || *text==0);
}

gint xfdashboard_text_box_get_length(XfdashboardTextBox *self)
{
	const gchar					*text;
	gint						textLength=0;

	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), 0);

	text=clutter_text_get_text(CLUTTER_TEXT(self->priv->actorTextBox));
	if(text) textLength=strlen(text);

	return(textLength);
}

const gchar* xfdashboard_text_box_get_text(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), NULL);

	return(clutter_text_get_text(CLUTTER_TEXT(self->priv->actorTextBox)));
}

void xfdashboard_text_box_set_text(XfdashboardTextBox *self, const gchar *inMarkupText)
{
	XfdashboardTextBoxPrivate	*priv;
	const gchar					*text;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(clutter_text_get_text(CLUTTER_TEXT(priv->actorTextBox)), inMarkupText)!=0)
	{
		clutter_text_set_markup(CLUTTER_TEXT(priv->actorTextBox), inMarkupText);

		text=clutter_text_get_text(CLUTTER_TEXT(priv->actorTextBox));
		if(text==NULL || *text==0)
		{
			clutter_actor_show(priv->actorHintLabel);
		}
			else
			{
				clutter_actor_hide(priv->actorHintLabel);
			}

		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_TEXT]);
	}
}

/* Get/set font of editable text box */
const gchar* xfdashboard_text_box_get_text_font(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), NULL);

	return(self->priv->actorTextBox!=NULL ? self->priv->textFont : NULL);
}

void xfdashboard_text_box_set_text_font(XfdashboardTextBox *self, const gchar *inFont)
{
	XfdashboardTextBoxPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->textFont, inFont)!=0)
	{
		if(priv->textFont) g_free(priv->textFont);
		priv->textFont=g_strdup(inFont);

		clutter_text_set_font_name(CLUTTER_TEXT(priv->actorTextBox), priv->textFont);
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_TEXT_FONT]);
	}
}

/* Get/set color of text in editable text box */
const ClutterColor* xfdashboard_text_box_get_text_color(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), NULL);

	return(self->priv->textColor);
}

void xfdashboard_text_box_set_text_color(XfdashboardTextBox *self, const ClutterColor *inColor)
{
	XfdashboardTextBoxPrivate	*priv;
	ClutterColor				selectionColor;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));
	g_return_if_fail(inColor);

	priv=self->priv;

	/* Set value if changed */
	if(!priv->textColor || !clutter_color_equal(inColor, priv->textColor))
	{
		if(priv->textColor) clutter_color_free(priv->textColor);
		priv->textColor=clutter_color_copy(inColor);

		clutter_text_set_color(CLUTTER_TEXT(priv->actorTextBox), priv->textColor);

		/* Selection text color is inverted text color */
		selectionColor.red=0xff-priv->textColor->red;
		selectionColor.green=0xff-priv->textColor->green;
		selectionColor.blue=0xff-priv->textColor->blue;
		selectionColor.alpha=priv->textColor->alpha;
		clutter_text_set_selected_text_color(CLUTTER_TEXT(priv->actorTextBox), &selectionColor);

		/* Selection color is the same as text color */
		clutter_text_set_selection_color(CLUTTER_TEXT(priv->actorTextBox), priv->textColor);

		/* Redraw actor in new color */
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_TEXT_COLOR]);
	}
}

/* Get/set text of hint label */
const gchar* xfdashboard_text_box_get_hint_text(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), NULL);

	return(clutter_text_get_text(CLUTTER_TEXT(self->priv->actorHintLabel)));
}

void xfdashboard_text_box_set_hint_text(XfdashboardTextBox *self, const gchar *inMarkupText)
{
	XfdashboardTextBoxPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(clutter_text_get_text(CLUTTER_TEXT(priv->actorHintLabel)), inMarkupText)!=0)
	{
		clutter_text_set_markup(CLUTTER_TEXT(priv->actorHintLabel), inMarkupText);
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_HINT_TEXT]);
	}
}

/* Get/set font of hint label */
const gchar* xfdashboard_text_box_get_hint_text_font(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), NULL);

	return(self->priv->hintTextFont);
}

void xfdashboard_text_box_set_hint_text_font(XfdashboardTextBox *self, const gchar *inFont)
{
	XfdashboardTextBoxPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->hintTextFont, inFont)!=0)
	{
		if(priv->hintTextFont) g_free(priv->hintTextFont);
		priv->hintTextFont=g_strdup(inFont);

		clutter_text_set_font_name(CLUTTER_TEXT(priv->actorHintLabel), priv->hintTextFont);
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_HINT_TEXT_FONT]);
	}
}

/* Get/set color of text in hint label */
const ClutterColor* xfdashboard_text_box_get_hint_text_color(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), NULL);

	return(self->priv->hintTextColor);
}

void xfdashboard_text_box_set_hint_text_color(XfdashboardTextBox *self, const ClutterColor *inColor)
{
	XfdashboardTextBoxPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));
	g_return_if_fail(inColor);

	priv=self->priv;

	/* Set value if changed */
	if(!priv->hintTextColor || !clutter_color_equal(inColor, priv->hintTextColor))
	{
		if(priv->hintTextColor) clutter_color_free(priv->hintTextColor);
		priv->hintTextColor=clutter_color_copy(inColor);

		clutter_text_set_color(CLUTTER_TEXT(priv->actorHintLabel), priv->hintTextColor);
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_HINT_TEXT_COLOR]);
	}
}

/* Get/set primary icon */
const gchar* xfdashboard_text_box_get_primary_icon(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), NULL);

	return(self->priv->primaryIconName);
}

void xfdashboard_text_box_set_primary_icon(XfdashboardTextBox *self, const gchar *inIconName)
{
	XfdashboardTextBoxPrivate	*priv;
	ClutterImage				*image;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));
	g_return_if_fail(!inIconName || strlen(inIconName)>0);

	priv=self->priv;

	/* Set themed icon name or icon file name for primary icon */
	if(g_strcmp0(priv->primaryIconName, inIconName)!=0)
	{
		/* Set new primary icon name */
		if(priv->primaryIconName)
		{
			g_free(priv->primaryIconName);
			priv->primaryIconName=NULL;
		}

		if(inIconName)
		{
			priv->primaryIconName=g_strdup(inIconName);

			/* Load and set new icon */
			image=xfdashboard_get_image_for_icon_name(priv->primaryIconName, DEFAULT_ICON_SIZE);
			clutter_actor_set_content(priv->actorPrimaryIcon, CLUTTER_CONTENT(image));
			g_object_unref(image);
		}

		/* If NULL or empty icon name was set hide primary icon */
		if(priv->primaryIconName==NULL || *priv->primaryIconName==0)
		{
			if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorPrimaryIcon))
			{
				clutter_actor_hide(priv->actorPrimaryIcon);
				clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
			}

			priv->showPrimaryIcon=FALSE;
		}
			/* Otherwise set new primary icon and show it */
			else
			{

				if(!CLUTTER_ACTOR_IS_VISIBLE(priv->actorPrimaryIcon))
				{
					clutter_actor_show(priv->actorPrimaryIcon);
					clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
				}

				priv->showPrimaryIcon=TRUE;
			}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_PRIMARY_ICON_NAME]);
	}
}

/* Get/set secondary icon */
const gchar* xfdashboard_text_box_get_secondary_icon(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), NULL);

	return(self->priv->secondaryIconName);
}

void xfdashboard_text_box_set_secondary_icon(XfdashboardTextBox *self, const gchar *inIconName)
{
	XfdashboardTextBoxPrivate	*priv;
	ClutterImage				*image;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));
	g_return_if_fail(!inIconName || strlen(inIconName)>0);

	priv=self->priv;

	/* Set themed icon name or icon file name for primary icon */
	if(g_strcmp0(priv->secondaryIconName, inIconName)!=0)
	{
		/* Set new primary icon name */
		if(priv->secondaryIconName)
		{
			g_free(priv->secondaryIconName);
			priv->secondaryIconName=NULL;
		}

		if(inIconName)
		{
			priv->secondaryIconName=g_strdup(inIconName);

			/* Load and set new icon */
			image=xfdashboard_get_image_for_icon_name(priv->secondaryIconName, DEFAULT_ICON_SIZE);
			clutter_actor_set_content(priv->actorSecondaryIcon, CLUTTER_CONTENT(image));
			g_object_unref(image);
		}

		/* If NULL or empty icon name was set hide primary icon */
		if(priv->secondaryIconName==NULL || *priv->secondaryIconName==0)
		{
			if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorSecondaryIcon))
			{
				clutter_actor_hide(priv->actorSecondaryIcon);
				clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
			}

			priv->showSecondaryIcon=FALSE;
		}
			/* Otherwise set new primary icon and show it */
			else
			{
				if(!CLUTTER_ACTOR_IS_VISIBLE(priv->actorSecondaryIcon))
				{
					clutter_actor_show(priv->actorSecondaryIcon);
					clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
				}

				priv->showSecondaryIcon=FALSE;
			}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_SECONDARY_ICON_NAME]);
	}
}