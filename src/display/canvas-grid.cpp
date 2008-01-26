#define INKSCAPE_CANVAS_GRID_C

/*
 *
 * Copyright (C) Johan Engelen 2006-2007 <johan@shouraizou.nl>
 * Copyright (C) Lauris Kaplinski 2000
 *
 */

/* As a general comment, I am not exactly proud of how things are done.
 * (for example the 'enable' widget and readRepr things)
 * It does seem to work however. I intend to clean up and sort things out later, but that can take forever...
 * Don't be shy to correct things.
 */


#include "sp-canvas-util.h"
#include "util/mathfns.h" 
#include "display-forward.h"
#include <libnr/nr-pixops.h>
#include "desktop-handles.h"
#include "helper/units.h"
#include "svg/svg-color.h"
#include "xml/node-event-vector.h"
#include "sp-object.h"

#include "sp-namedview.h"
#include "inkscape.h"
#include "desktop.h"

#include "../document.h"
#include "prefs-utils.h"

#include "canvas-grid.h"
#include "canvas-axonomgrid.h"

namespace Inkscape {

static gchar const *const grid_name[] = {
    N_("Rectangular grid"),
    N_("Axonometric grid")
};
static gchar const *const grid_svgname[] = {
    "xygrid",
    "axonomgrid"
};


// ##########################################################
// Grid CanvasItem
static void grid_canvasitem_class_init (GridCanvasItemClass *klass);
static void grid_canvasitem_init (GridCanvasItem *grid);
static void grid_canvasitem_destroy (GtkObject *object);

static void grid_canvasitem_update (SPCanvasItem *item, NR::Matrix const &affine, unsigned int flags);
static void grid_canvasitem_render (SPCanvasItem *item, SPCanvasBuf *buf);

static SPCanvasItemClass * parent_class;

GtkType
grid_canvasitem_get_type (void)
{
    static GtkType grid_canvasitem_type = 0;

    if (!grid_canvasitem_type) {
        GtkTypeInfo grid_canvasitem_info = {
            "GridCanvasItem",
            sizeof (GridCanvasItem),
            sizeof (GridCanvasItemClass),
            (GtkClassInitFunc) grid_canvasitem_class_init,
            (GtkObjectInitFunc) grid_canvasitem_init,
            NULL, NULL,
            (GtkClassInitFunc) NULL
        };
        grid_canvasitem_type = gtk_type_unique (sp_canvas_item_get_type (), &grid_canvasitem_info);
    }
    return grid_canvasitem_type;
}

static void
grid_canvasitem_class_init (GridCanvasItemClass *klass)
{
    GtkObjectClass *object_class;
    SPCanvasItemClass *item_class;

    object_class = (GtkObjectClass *) klass;
    item_class = (SPCanvasItemClass *) klass;

    parent_class = (SPCanvasItemClass*)gtk_type_class (sp_canvas_item_get_type ());

    object_class->destroy = grid_canvasitem_destroy;

    item_class->update = grid_canvasitem_update;
    item_class->render = grid_canvasitem_render;
}

static void
grid_canvasitem_init (GridCanvasItem *griditem)
{
    griditem->grid = NULL;
}

static void
grid_canvasitem_destroy (GtkObject *object)
{
    g_return_if_fail (object != NULL);
    g_return_if_fail (INKSCAPE_IS_GRID_CANVASITEM (object));

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
        (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/**
*/
static void
grid_canvasitem_render (SPCanvasItem * item, SPCanvasBuf * buf)
{
    GridCanvasItem *gridcanvasitem = INKSCAPE_GRID_CANVASITEM (item);

    if ( gridcanvasitem->grid && gridcanvasitem->grid->isVisible() ) {
        sp_canvas_prepare_buffer (buf);
        gridcanvasitem->grid->Render(buf);
    }
}

static void
grid_canvasitem_update (SPCanvasItem *item, NR::Matrix const &affine, unsigned int flags)
{
    GridCanvasItem *gridcanvasitem = INKSCAPE_GRID_CANVASITEM (item);

    if (parent_class->update)
        (* parent_class->update) (item, affine, flags);

    if (gridcanvasitem->grid) {
        gridcanvasitem->grid->Update(affine, flags);

        sp_canvas_request_redraw (item->canvas,
                         -1000000, -1000000,
                         1000000, 1000000);

        item->x1 = item->y1 = -1000000;
        item->x2 = item->y2 = 1000000;
    }
}



// ##########################################################
//   CanvasGrid

    static Inkscape::XML::NodeEventVector const _repr_events = {
        NULL, /* child_added */
        NULL, /* child_removed */
        CanvasGrid::on_repr_attr_changed,
        NULL, /* content_changed */
        NULL  /* order_changed */
    };

CanvasGrid::CanvasGrid(SPNamedView * nv, Inkscape::XML::Node * in_repr, SPDocument *in_doc, GridType type)
    : visible(true), gridtype(type)
{
    repr = in_repr;
    doc = in_doc;
    if (repr) {
        repr->addListener (&_repr_events, this);
    }

    namedview = nv;
    canvasitems = NULL;
}

CanvasGrid::~CanvasGrid()
{
    if (repr) {
        repr->removeListenerByData (this);
    }

    while (canvasitems) {
        gtk_object_destroy(GTK_OBJECT(canvasitems->data));
        canvasitems = g_slist_remove(canvasitems, canvasitems->data);
    }
}

const char *
CanvasGrid::getName()
{
    return _(grid_name[gridtype]);
}

const char *
CanvasGrid::getSVGName()
{
    return grid_svgname[gridtype];
}

GridType
CanvasGrid::getGridType()
{
    return gridtype;
}


char const *
CanvasGrid::getName(GridType type)
{
    return _(grid_name[type]);
}

char const *
CanvasGrid::getSVGName(GridType type)
{
    return grid_svgname[type];
}

GridType
CanvasGrid::getGridTypeFromSVGName(char const *typestr)
{
    if (!typestr) return GRID_RECTANGULAR;

    gint t = 0;
    for (t = GRID_MAXTYPENR; t >= 0; t--) {  //this automatically defaults to grid0 which is rectangular grid
        if (!strcmp(typestr, grid_svgname[t])) break;
    }
    return (GridType) t;
}

GridType
CanvasGrid::getGridTypeFromName(char const *typestr)
{
    if (!typestr) return GRID_RECTANGULAR;

    gint t = 0;
    for (t = GRID_MAXTYPENR; t >= 0; t--) {  //this automatically defaults to grid0 which is rectangular grid
        if (!strcmp(typestr, _(grid_name[t]))) break;
    }
    return (GridType) t;
}


/*
*  writes an <inkscape:grid> child to repr.
*/
void
CanvasGrid::writeNewGridToRepr(Inkscape::XML::Node * repr, SPDocument * doc, GridType gridtype)
{
    if (!repr) return;
    if (gridtype > GRID_MAXTYPENR) return;

    // first create the child xml node, then hook it to repr. This order is important, to not set off listeners to repr before the new node is complete.

    Inkscape::XML::Document *xml_doc = sp_document_repr_doc(doc);
    Inkscape::XML::Node *newnode;
    newnode = xml_doc->createElement("inkscape:grid");
    newnode->setAttribute("type", getSVGName(gridtype));

    repr->appendChild(newnode);
    Inkscape::GC::release(newnode);

    sp_document_done(doc, SP_VERB_DIALOG_NAMEDVIEW, _("Create new grid"));
}

/*
* Creates a new CanvasGrid object of type gridtype
*/
CanvasGrid*
CanvasGrid::NewGrid(SPNamedView * nv, Inkscape::XML::Node * repr, SPDocument * doc, GridType gridtype)
{
    if (!repr) return NULL;
    if (!doc) {
        g_error("CanvasGrid::NewGrid - doc==NULL");
        return NULL;
    }

    switch (gridtype) {
        case GRID_RECTANGULAR:
            return (CanvasGrid*) new CanvasXYGrid(nv, repr, doc);
        case GRID_AXONOMETRIC:
            return (CanvasGrid*) new CanvasAxonomGrid(nv, repr, doc);
    }

    return NULL;
}


/**
*  creates a new grid canvasitem for the SPDesktop given as parameter. Keeps a link to this canvasitem in the canvasitems list.
*/
GridCanvasItem *
CanvasGrid::createCanvasItem(SPDesktop * desktop)
{
    if (!desktop) return NULL;
//    Johan: I think for multiple desktops it is best if each has their own canvasitem,
//           but share the same CanvasGrid object; that is what this function is for.

    // check if there is already a canvasitem on this desktop linking to this grid
    for (GSList *l = canvasitems; l != NULL; l = l->next) {
        if ( sp_desktop_gridgroup(desktop) == SP_CANVAS_GROUP(SP_CANVAS_ITEM(l->data)->parent) ) {
            return NULL;
        }
    }

    GridCanvasItem * item = INKSCAPE_GRID_CANVASITEM( sp_canvas_item_new(sp_desktop_gridgroup(desktop), INKSCAPE_TYPE_GRID_CANVASITEM, NULL) );
    item->grid = this;
    sp_canvas_item_show(SP_CANVAS_ITEM(item));

    gtk_object_ref(GTK_OBJECT(item));    // since we're keeping a link to this item, we need to bump up the ref count
    canvasitems = g_slist_prepend(canvasitems, item);

    return item;
}

Gtk::Widget *
CanvasGrid::newWidget()
{
    Gtk::VBox * vbox = Gtk::manage( new Gtk::VBox() );
    Gtk::Label * namelabel = Gtk::manage(new Gtk::Label("", Gtk::ALIGN_CENTER) );

    Glib::ustring str("<b>");
    str += getName();
    str += "</b>";
    namelabel->set_markup(str);
    vbox->pack_start(*namelabel, true, true);

    Inkscape::UI::Widget::RegisteredCheckButton * _rcb_enabled = Gtk::manage(
        new Inkscape::UI::Widget::RegisteredCheckButton( _("_Enabled"),
                        _("Determines whether to snap to this grid or not. Can be 'on' for invisible grids."),
                         "enabled", _wr, false, repr, doc) );
    Inkscape::UI::Widget::RegisteredCheckButton * _rcb_visible = Gtk::manage(
        new Inkscape::UI::Widget::RegisteredCheckButton( _("_Visible"),
                        _("Determines whether the grid is displayed or not. Objects are still snapped to invisible grids."),
                         "visible", _wr, true, repr, doc) );

    vbox->pack_start(*_rcb_enabled, true, true);
    vbox->pack_start(*_rcb_visible, true, true);
    vbox->pack_start(*newSpecificWidget(), true, true);

    // set widget values
    _rcb_visible->setActive(visible);
    if (snapper != NULL) {
        _rcb_enabled->setActive(snapper->getEnabled());
    }

    return dynamic_cast<Gtk::Widget *> (vbox);
}

void
CanvasGrid::on_repr_attr_changed(Inkscape::XML::Node *repr, gchar const *key, gchar const *oldval, gchar const *newval, bool is_interactive, void *data)
{
    if (!data)
        return;

    ((CanvasGrid*) data)->onReprAttrChanged(repr, key, oldval, newval, is_interactive);
}

bool CanvasGrid::isEnabled() 
{ 
    if (snapper == NULL) {
       return false;
    } 
    
    return snapper->getEnabled();    
}

// ##########################################################
//   CanvasXYGrid


/**
* "attach_all" function
* A DIRECT COPY-PASTE FROM DOCUMENT-PROPERTIES.CPP  TO QUICKLY GET RESULTS
*
 * Helper function that attachs widgets in a 3xn table. The widgets come in an
 * array that has two entries per table row. The two entries code for four
 * possible cases: (0,0) means insert space in first column; (0, non-0) means
 * widget in columns 2-3; (non-0, 0) means label in columns 1-3; and
 * (non-0, non-0) means two widgets in columns 2 and 3.
**/
#define SPACE_SIZE_X 15
#define SPACE_SIZE_Y 10
static inline void
attach_all(Gtk::Table &table, Gtk::Widget const *const arr[], unsigned size, int start = 0)
{
    for (unsigned i=0, r=start; i<size/sizeof(Gtk::Widget*); i+=2) {
        if (arr[i] && arr[i+1]) {
            table.attach (const_cast<Gtk::Widget&>(*arr[i]),   1, 2, r, r+1,
                          Gtk::FILL|Gtk::EXPAND, (Gtk::AttachOptions)0,0,0);
            table.attach (const_cast<Gtk::Widget&>(*arr[i+1]), 2, 3, r, r+1,
                          Gtk::FILL|Gtk::EXPAND, (Gtk::AttachOptions)0,0,0);
        } else {
            if (arr[i+1]) {
                table.attach (const_cast<Gtk::Widget&>(*arr[i+1]), 1, 3, r, r+1,
                              Gtk::FILL|Gtk::EXPAND, (Gtk::AttachOptions)0,0,0);
            } else if (arr[i]) {
                Gtk::Label& label = reinterpret_cast<Gtk::Label&> (const_cast<Gtk::Widget&>(*arr[i]));
                label.set_alignment (0.0);
                table.attach (label, 0, 3, r, r+1,
                              Gtk::FILL|Gtk::EXPAND, (Gtk::AttachOptions)0,0,0);
            } else {
                Gtk::HBox *space = manage (new Gtk::HBox);
                space->set_size_request (SPACE_SIZE_X, SPACE_SIZE_Y);
                table.attach (*space, 0, 1, r, r+1,
                              (Gtk::AttachOptions)0, (Gtk::AttachOptions)0,0,0);
            }
        }
        ++r;
    }
}

CanvasXYGrid::CanvasXYGrid (SPNamedView * nv, Inkscape::XML::Node * in_repr, SPDocument * in_doc)
    : CanvasGrid(nv, in_repr, in_doc, GRID_RECTANGULAR)
{
    gridunit = sp_unit_get_by_abbreviation( prefs_get_string_attribute("options.grids.xy", "units") );
    if (!gridunit)
        gridunit = &sp_unit_get_by_id(SP_UNIT_PX);
    origin[NR::X] = sp_units_get_pixels( prefs_get_double_attribute ("options.grids.xy", "origin_x", 0.0), *(gridunit) );
    origin[NR::Y] = sp_units_get_pixels( prefs_get_double_attribute ("options.grids.xy", "origin_y", 0.0), *(gridunit) );
    color = prefs_get_int_attribute("options.grids.xy", "color", 0x0000ff20);
    empcolor = prefs_get_int_attribute("options.grids.xy", "empcolor", 0x0000ff40);
    empspacing = prefs_get_int_attribute("options.grids.xy", "empspacing", 5);
    spacing[NR::X] = sp_units_get_pixels( prefs_get_double_attribute ("options.grids.xy", "spacing_x", 0.0), *(gridunit) );
    spacing[NR::Y] = sp_units_get_pixels( prefs_get_double_attribute ("options.grids.xy", "spacing_y", 0.0), *(gridunit) );
    render_dotted = prefs_get_int_attribute ("options.grids.xy", "dotted", 0) == 1;

    snapper = new CanvasXYGridSnapper(this, namedview, 0);
}

CanvasXYGrid::~CanvasXYGrid ()
{
   if (snapper) delete snapper;
}


/* fixme: Collect all these length parsing methods and think common sane API */

static gboolean
sp_nv_read_length(gchar const *str, guint base, gdouble *val, SPUnit const **unit)
{
    if (!str) {
        return FALSE;
    }

    gchar *u;
    gdouble v = g_ascii_strtod(str, &u);
    if (!u) {
        return FALSE;
    }
    while (isspace(*u)) {
        u += 1;
    }

    if (!*u) {
        /* No unit specified - keep default */
        *val = v;
        return TRUE;
    }

    if (base & SP_UNIT_DEVICE) {
        if (u[0] && u[1] && !isalnum(u[2]) && !strncmp(u, "px", 2)) {
            *unit = &sp_unit_get_by_id(SP_UNIT_PX);
            *val = v;
            return TRUE;
        }
    }

    if (base & SP_UNIT_ABSOLUTE) {
        if (!strncmp(u, "pt", 2)) {
            *unit = &sp_unit_get_by_id(SP_UNIT_PT);
        } else if (!strncmp(u, "mm", 2)) {
            *unit = &sp_unit_get_by_id(SP_UNIT_MM);
        } else if (!strncmp(u, "cm", 2)) {
            *unit = &sp_unit_get_by_id(SP_UNIT_CM);
        } else if (!strncmp(u, "m", 1)) {
            *unit = &sp_unit_get_by_id(SP_UNIT_M);
        } else if (!strncmp(u, "in", 2)) {
            *unit = &sp_unit_get_by_id(SP_UNIT_IN);
        } else {
            return FALSE;
        }
        *val = v;
        return TRUE;
    }

    return FALSE;
}

static gboolean sp_nv_read_opacity(gchar const *str, guint32 *color)
{
    if (!str) {
        return FALSE;
    }

    gchar *u;
    gdouble v = g_ascii_strtod(str, &u);
    if (!u) {
        return FALSE;
    }
    v = CLAMP(v, 0.0, 1.0);

    *color = (*color & 0xffffff00) | (guint32) floor(v * 255.9999);

    return TRUE;
}

/** If the passed scalar is invalid (<=0), then set the widget and the scalar
    to use the given old value.

    @param oldVal Old value to use if the new one is invalid.
    @param pTarget The scalar to validate.
    @param widget Widget associated with the scalar.
*/
static void validateScalar(double oldVal,
                           double* pTarget)
{
    // Avoid nullness.
    if ( pTarget == NULL )
        return;

    // Invalid new value?
    if ( *pTarget <= 0 ) {
        // If the old value is somehow invalid as well, then default to 1.
        if ( oldVal <= 0 )
            oldVal = 1;

        // Reset the scalar and associated widget to the old value.
        *pTarget = oldVal;
    } //if

} //validateScalar


/** If the passed int is invalid (<=0), then set the widget and the int
    to use the given old value.

    @param oldVal Old value to use if the new one is invalid.
    @param pTarget The int to validate.
    @param widget Widget associated with the int.
*/
static void validateInt(gint oldVal,
                        gint* pTarget)
{
    // Avoid nullness.
    if ( pTarget == NULL )
        return;

    // Invalid new value?
    if ( *pTarget <= 0 ) {
        // If the old value is somehow invalid as well, then default to 1.
        if ( oldVal <= 0 )
            oldVal = 1;

        // Reset the int and associated widget to the old value.
        *pTarget = oldVal;
    } //if

} //validateInt

void
CanvasXYGrid::readRepr()
{
    gchar const *value;
    if ( (value = repr->attribute("originx")) ) {
        sp_nv_read_length(value, SP_UNIT_ABSOLUTE | SP_UNIT_DEVICE, &origin[NR::X], &gridunit);
        origin[NR::X] = sp_units_get_pixels(origin[NR::X], *(gridunit));
    }

    if ( (value = repr->attribute("originy")) ) {
        sp_nv_read_length(value, SP_UNIT_ABSOLUTE | SP_UNIT_DEVICE, &origin[NR::Y], &gridunit);
        origin[NR::Y] = sp_units_get_pixels(origin[NR::Y], *(gridunit));
    }

    if ( (value = repr->attribute("spacingx")) ) {
        double oldVal = spacing[NR::X];
        sp_nv_read_length(value, SP_UNIT_ABSOLUTE | SP_UNIT_DEVICE, &spacing[NR::X], &gridunit);
        validateScalar( oldVal, &spacing[NR::X]);
        spacing[NR::X] = sp_units_get_pixels(spacing[NR::X], *(gridunit));

    }
    if ( (value = repr->attribute("spacingy")) ) {
        double oldVal = spacing[NR::Y];
        sp_nv_read_length(value, SP_UNIT_ABSOLUTE | SP_UNIT_DEVICE, &spacing[NR::Y], &gridunit);
        validateScalar( oldVal, &spacing[NR::Y]);
        spacing[NR::Y] = sp_units_get_pixels(spacing[NR::Y], *(gridunit));

    }

    if ( (value = repr->attribute("color")) ) {
        color = (color & 0xff) | sp_svg_read_color(value, color);
    }

    if ( (value = repr->attribute("empcolor")) ) {
        empcolor = (empcolor & 0xff) | sp_svg_read_color(value, empcolor);
    }

    if ( (value = repr->attribute("opacity")) ) {
        sp_nv_read_opacity(value, &color);
    }
    if ( (value = repr->attribute("empopacity")) ) {
        sp_nv_read_opacity(value, &empcolor);
    }

    if ( (value = repr->attribute("empspacing")) ) {
        gint oldVal = empspacing;
        empspacing = atoi(value);
        validateInt( oldVal, &empspacing);
    }

    if ( (value = repr->attribute("dotted")) ) {
        render_dotted = (strcmp(value,"true") == 0);
    }

    if ( (value = repr->attribute("visible")) ) {
        visible = (strcmp(value,"true") == 0);
    }
    
    if ( (value = repr->attribute("enabled")) ) {
        g_assert(snapper != NULL);
        snapper->setEnabled(strcmp(value,"true") == 0);
    }

    for (GSList *l = canvasitems; l != NULL; l = l->next) {
        sp_canvas_item_request_update ( SP_CANVAS_ITEM(l->data) );
    }

    return;
}

/**
 * Called when XML node attribute changed; updates dialog widgets if change was not done by widgets themselves.
 */
void
CanvasXYGrid::onReprAttrChanged(Inkscape::XML::Node */*repr*/, gchar const */*key*/, gchar const */*oldval*/, gchar const */*newval*/, bool /*is_interactive*/)
{
    readRepr();

    if ( ! (_wr.isUpdating()) )
        updateWidgets();
}




Gtk::Widget *
CanvasXYGrid::newSpecificWidget()
{
    Gtk::Table * table = Gtk::manage( new Gtk::Table(1,1) );

    Inkscape::UI::Widget::RegisteredUnitMenu *_rumg = new Inkscape::UI::Widget::RegisteredUnitMenu();
    Inkscape::UI::Widget::RegisteredScalarUnit *_rsu_ox = new Inkscape::UI::Widget::RegisteredScalarUnit();
    Inkscape::UI::Widget::RegisteredScalarUnit *_rsu_oy = new Inkscape::UI::Widget::RegisteredScalarUnit();
    Inkscape::UI::Widget::RegisteredScalarUnit *_rsu_sx = new Inkscape::UI::Widget::RegisteredScalarUnit();
    Inkscape::UI::Widget::RegisteredScalarUnit *_rsu_sy = new Inkscape::UI::Widget::RegisteredScalarUnit();

    Inkscape::UI::Widget::RegisteredColorPicker *_rcp_gcol = Gtk::manage(
        new Inkscape::UI::Widget::RegisteredColorPicker(
            _("Grid line _color:"), _("Grid line color"), _("Color of grid lines"), 
            "color", "opacity", _wr, repr, doc));

    Inkscape::UI::Widget::RegisteredColorPicker *_rcp_gmcol = Gtk::manage(
        new Inkscape::UI::Widget::RegisteredColorPicker(
            _("Ma_jor grid line color:"), _("Major grid line color"), 
            _("Color of the major (highlighted) grid lines"), "empcolor", "empopacity", 
            _wr, repr, doc));
    
    Inkscape::UI::Widget::RegisteredSuffixedInteger *_rsi = new Inkscape::UI::Widget::RegisteredSuffixedInteger();

    // initialize widgets:
    table->set_spacings(2);

_wr.setUpdating (true);
    Inkscape::UI::Widget::ScalarUnit * sutemp;
    _rumg->init (_("Grid _units:"), "units", _wr, repr, doc);
    _rsu_ox->init (_("_Origin X:"), _("X coordinate of grid origin"),
                  "originx", *_rumg, _wr, repr, doc);
        sutemp = _rsu_ox->getSU();
        sutemp->setDigits(4);
        sutemp->setIncrements(0.1, 1.0);
    _rsu_oy->init (_("O_rigin Y:"), _("Y coordinate of grid origin"),
                  "originy", *_rumg, _wr, repr, doc);
        sutemp = _rsu_oy->getSU();
        sutemp->setDigits(4);
        sutemp->setIncrements(0.1, 1.0);
    _rsu_sx->init (_("Spacing _X:"), _("Distance between vertical grid lines"),
                  "spacingx", *_rumg, _wr, repr, doc);
        sutemp = _rsu_sx->getSU();
        sutemp->setDigits(4);
        sutemp->setIncrements(0.1, 1.0);
    _rsu_sy->init (_("Spacing _Y:"), _("Distance between horizontal grid lines"),
                  "spacingy", *_rumg, _wr, repr, doc);
        sutemp = _rsu_sy->getSU();
        sutemp->setDigits(4);
        sutemp->setIncrements(0.1, 1.0);

    _rsi->init (_("_Major grid line every:"), _("lines"), "empspacing", _wr, repr, doc);

    Inkscape::UI::Widget::RegisteredCheckButton * _rcb_dotted = Gtk::manage(
                new Inkscape::UI::Widget::RegisteredCheckButton( _("_Show dots instead of lines"),
                       _("If set, displays dots at gridpoints instead of gridlines"),
                        "dotted", _wr, false, repr, doc) 
                );
_wr.setUpdating (false);

    Gtk::Widget const *const widget_array[] = {
        _rumg->_label,       _rumg->_sel,
        0,                  _rsu_ox->getSU(),
        0,                  _rsu_oy->getSU(),
        0,                  _rsu_sx->getSU(),
        0,                  _rsu_sy->getSU(),
        _rcp_gcol->_label,   _rcp_gcol,
        0,                  0,
        _rcp_gmcol->_label,  _rcp_gmcol,
        _rsi->_label,        &_rsi->_hbox,
        0,                  _rcb_dotted,
    };

    attach_all (*table, widget_array, sizeof(widget_array));

    if (repr) readRepr();

    // set widget values
    _rumg->setUnit (gridunit);

    gdouble val;
    val = origin[NR::X];
    val = sp_pixels_get_units (val, *(gridunit));
    _rsu_ox->setValue (val);
    val = origin[NR::Y];
    val = sp_pixels_get_units (val, *(gridunit));
    _rsu_oy->setValue (val);
    val = spacing[NR::X];
    double gridx = sp_pixels_get_units (val, *(gridunit));
    _rsu_sx->setValue (gridx);
    val = spacing[NR::Y];
    double gridy = sp_pixels_get_units (val, *(gridunit));
    _rsu_sy->setValue (gridy);

    _rcp_gcol->setRgba32 (color);
    _rcp_gmcol->setRgba32 (empcolor);
    _rsi->setValue (empspacing);

    _rcb_dotted->setActive(render_dotted);

    return table;
}


/**
 * Update dialog widgets from object's values.
 */
void
CanvasXYGrid::updateWidgets()
{
/*
    if (_wr.isUpdating()) return;

    _wr.setUpdating (true);

    _rcb_visible.setActive(visible);
    if (snapper != NULL) {
        _rcb_enabled.setActive(snapper->getEnabled());
    }

    _rumg.setUnit (gridunit);

    gdouble val;
    val = origin[NR::X];
    val = sp_pixels_get_units (val, *(gridunit));
    _rsu_ox.setValue (val);
    val = origin[NR::Y];
    val = sp_pixels_get_units (val, *(gridunit));
    _rsu_oy.setValue (val);
    val = spacing[NR::X];
    double gridx = sp_pixels_get_units (val, *(gridunit));
    _rsu_sx.setValue (gridx);
    val = spacing[NR::Y];
    double gridy = sp_pixels_get_units (val, *(gridunit));
    _rsu_sy.setValue (gridy);

    _rcp_gcol.setRgba32 (color);
    _rcp_gmcol.setRgba32 (empcolor);
    _rsi.setValue (empspacing);

    _rcb_dotted.setActive(render_dotted);

    _wr.setUpdating (false);

    return;
*/
}



void
CanvasXYGrid::Update (NR::Matrix const &affine, unsigned int /*flags*/)
{
    ow = origin * affine;
    sw = spacing * affine;
    sw -= NR::Point(affine[4], affine[5]);

    for(int dim = 0; dim < 2; dim++) {
        gint scaling_factor = empspacing;

        if (scaling_factor <= 1)
            scaling_factor = 5;

        scaled[dim] = FALSE;
        sw[dim] = fabs (sw[dim]);
        while (sw[dim] < 8.0) {
            scaled[dim] = TRUE;
            sw[dim] *= scaling_factor;
            /* First pass, go up to the major line spacing, then
               keep increasing by two. */
            scaling_factor = 2;
        }
    }
}


static void
grid_hline (SPCanvasBuf *buf, gint y, gint xs, gint xe, guint32 rgba)
{
    if ((y >= buf->rect.y0) && (y < buf->rect.y1)) {
        guint r, g, b, a;
        gint x0, x1, x;
        guchar *p;
        r = NR_RGBA32_R (rgba);
        g = NR_RGBA32_G (rgba);
        b = NR_RGBA32_B (rgba);
        a = NR_RGBA32_A (rgba);
        x0 = MAX (buf->rect.x0, xs);
        x1 = MIN (buf->rect.x1, xe + 1);
        p = buf->buf + (y - buf->rect.y0) * buf->buf_rowstride + (x0 - buf->rect.x0) * 3;
        for (x = x0; x < x1; x++) {
            p[0] = NR_COMPOSEN11_1111 (r, a, p[0]);
            p[1] = NR_COMPOSEN11_1111 (g, a, p[1]);
            p[2] = NR_COMPOSEN11_1111 (b, a, p[2]);
            p += 3;
        }
    }
}

static void
grid_vline (SPCanvasBuf *buf, gint x, gint ys, gint ye, guint32 rgba)
{
    if ((x >= buf->rect.x0) && (x < buf->rect.x1)) {
        guint r, g, b, a;
        gint y0, y1, y;
        guchar *p;
        r = NR_RGBA32_R(rgba);
        g = NR_RGBA32_G (rgba);
        b = NR_RGBA32_B (rgba);
        a = NR_RGBA32_A (rgba);
        y0 = MAX (buf->rect.y0, ys);
        y1 = MIN (buf->rect.y1, ye + 1);
        p = buf->buf + (y0 - buf->rect.y0) * buf->buf_rowstride + (x - buf->rect.x0) * 3;
        for (y = y0; y < y1; y++) {
            p[0] = NR_COMPOSEN11_1111 (r, a, p[0]);
            p[1] = NR_COMPOSEN11_1111 (g, a, p[1]);
            p[2] = NR_COMPOSEN11_1111 (b, a, p[2]);
            p += buf->buf_rowstride;
        }
    }
}

static void
grid_dot (SPCanvasBuf *buf, gint x, gint y, guint32 rgba)
{
    if ( (y >= buf->rect.y0) && (y < buf->rect.y1)
         && (x >= buf->rect.x0) && (x < buf->rect.x1) ) {
        guint r, g, b, a;
        guchar *p;
        r = NR_RGBA32_R (rgba);
        g = NR_RGBA32_G (rgba);
        b = NR_RGBA32_B (rgba);
        a = NR_RGBA32_A (rgba);
        p = buf->buf + (y - buf->rect.y0) * buf->buf_rowstride + (x - buf->rect.x0) * 3;
        p[0] = NR_COMPOSEN11_1111 (r, a, p[0]);
        p[1] = NR_COMPOSEN11_1111 (g, a, p[1]);
        p[2] = NR_COMPOSEN11_1111 (b, a, p[2]);
    }
}

void
CanvasXYGrid::Render (SPCanvasBuf *buf)
{
    gdouble const sxg = floor ((buf->rect.x0 - ow[NR::X]) / sw[NR::X]) * sw[NR::X] + ow[NR::X];
    gint const  xlinestart = (gint) Inkscape::round((sxg - ow[NR::X]) / sw[NR::X]);
    gdouble const syg = floor ((buf->rect.y0 - ow[NR::Y]) / sw[NR::Y]) * sw[NR::Y] + ow[NR::Y];
    gint const  ylinestart = (gint) Inkscape::round((syg - ow[NR::Y]) / sw[NR::Y]);

    //set correct coloring, depending preference (when zoomed out, always major coloring or minor coloring)
    guint32 _empcolor;
    bool preference = prefs_get_int_attribute ("options.grids", "no_emphasize_when_zoomedout", 0) == 1;
    if( (scaled[NR::X] || scaled[NR::Y]) && preference ) {
        _empcolor = color;
    } else {
        _empcolor = empcolor;
    }

    if (!render_dotted) {
        gint ylinenum;
        gdouble y;
        for (y = syg, ylinenum = ylinestart; y < buf->rect.y1; y += sw[NR::Y], ylinenum++) {
            gint const y0 = (gint) Inkscape::round(y);
            if (!scaled[NR::Y] && (ylinenum % empspacing) != 0) {
                grid_hline (buf, y0, buf->rect.x0, buf->rect.x1 - 1, color);
            } else {
                grid_hline (buf, y0, buf->rect.x0, buf->rect.x1 - 1, _empcolor);
            }
        }

        gint xlinenum;
        gdouble x;
        for (x = sxg, xlinenum = xlinestart; x < buf->rect.x1; x += sw[NR::X], xlinenum++) {
            gint const ix = (gint) Inkscape::round(x);
            if (!scaled[NR::X] && (xlinenum % empspacing) != 0) {
                grid_vline (buf, ix, buf->rect.y0, buf->rect.y1, color);
            } else {
                grid_vline (buf, ix, buf->rect.y0, buf->rect.y1, _empcolor);
            }
        }
    } else {
        gint ylinenum;
        gdouble y;
        for (y = syg, ylinenum = ylinestart; y < buf->rect.y1; y += sw[NR::Y], ylinenum++) {
            gint const iy = (gint) Inkscape::round(y);

            gint xlinenum;
            gdouble x;
            for (x = sxg, xlinenum = xlinestart; x < buf->rect.x1; x += sw[NR::X], xlinenum++) {
                gint const ix = (gint) Inkscape::round(x);
                if ( (!scaled[NR::X] && (xlinenum % empspacing) == 0)
                     || (!scaled[NR::Y] && (ylinenum % empspacing) == 0) )
                {
                    grid_dot (buf, ix, iy, _empcolor | (guint32)0x000000FF); // put alpha to max value
                } else {
                    grid_dot (buf, ix, iy, color | (guint32)0x000000FF);  // put alpha to max value
                }
            }

        }
    }
}

CanvasXYGridSnapper::CanvasXYGridSnapper(CanvasXYGrid *grid, SPNamedView const *nv, NR::Coord const d) : LineSnapper(nv, d)
{
    this->grid = grid;
}

LineSnapper::LineList
CanvasXYGridSnapper::_getSnapLines(NR::Point const &p) const
{
    LineList s;

    if ( grid == NULL ) {
        return s;
    }

    for (unsigned int i = 0; i < 2; ++i) {

        /* This is to make sure we snap to only visible grid lines */
        double scaled_spacing = grid->sw[i]; // this is spacing of visible lines if screen pixels

        // convert screen pixels to px
        // FIXME: after we switch to snapping dist in screen pixels, this will be unnecessary
        if (SP_ACTIVE_DESKTOP) {
            scaled_spacing /= SP_ACTIVE_DESKTOP->current_zoom();
        }

        NR::Coord rounded;        
        NR::Point point_on_line;
        
        rounded = Inkscape::Util::round_to_upper_multiple_plus(p[i], scaled_spacing, grid->origin[i]);
        point_on_line = i ? NR::Point(0, rounded) : NR::Point(rounded, 0);
        s.push_back(std::make_pair(component_vectors[i], point_on_line));
        
        rounded = Inkscape::Util::round_to_lower_multiple_plus(p[i], scaled_spacing, grid->origin[i]);
        point_on_line = i ? NR::Point(0, rounded) : NR::Point(rounded, 0);
        s.push_back(std::make_pair(component_vectors[i], point_on_line));
    }

    return s;
}

void CanvasXYGridSnapper::_addSnappedLine(SnappedConstraints &sc, NR::Point const snapped_point, NR::Coord const snapped_distance, NR::Point const normal_to_line, NR::Point const point_on_line) const 
{
    SnappedLine dummy = SnappedLine(snapped_point, snapped_distance, getSnapperTolerance(), getSnapperAlwaysSnap(), normal_to_line, point_on_line);
    sc.grid_lines.push_back(dummy);
}

/**
 *  \return true if this Snapper will snap at least one kind of point.
 */
bool CanvasXYGridSnapper::ThisSnapperMightSnap() const
{
    return _named_view == NULL ? false : (_snap_enabled && _snap_from != 0);
}





}; /* namespace Inkscape */


/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
