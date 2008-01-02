#define INKSCAPE_LIVEPATHEFFECT_CPP

/*
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/effect.h"

#include "display/display-forward.h"
#include "xml/node-event-vector.h"
#include "sp-object.h"
#include "attributes.h"
#include "message-stack.h"
#include "desktop.h"
#include "inkscape.h"
#include "document.h"
#include <glibmm/i18n.h>

#include "live_effects/lpeobject.h"
#include "live_effects/parameter/parameter.h"
#include <glibmm/ustring.h>
#include "live_effects/n-art-bpath-2geom.h"
#include "display/curve.h"
#include <gtkmm.h>

#include <exception>

#include <2geom/sbasis-to-bezier.h>
#include <2geom/matrix.h>


// include effects:
#include "live_effects/lpe-skeletalstrokes.h"
#include "live_effects/lpe-pathalongpath.h"
#include "live_effects/lpe-slant.h"
#include "live_effects/lpe-test-doEffect-stack.h"
#include "live_effects/lpe-gears.h"
#include "live_effects/lpe-curvestitch.h"

#include "nodepath.h"

namespace Inkscape {

namespace LivePathEffect {

const Util::EnumData<EffectType> LPETypeData[INVALID_LPE] = {
    // {constant defined in effect.h, N_("name of your effect"), "name of your effect in SVG"}
    {PATH_ALONG_PATH,       N_("Bend Path"),             "bend_path"},
    {SKELETAL_STROKES,      N_("Pattern along path"),    "skeletal"},
#ifdef LPE_ENABLE_TEST_EFFECTS
    {SLANT,                 N_("Slant"),                 "slant"},
    {DOEFFECTSTACK_TEST,    N_("doEffect stack test"),   "doeffectstacktest"},
#endif
    {GEARS,                 N_("Gears"),                 "gears"},
    {CURVE_STITCH,          N_("Stitch subpaths"),       "curvestitching"},
};
const Util::EnumDataConverter<EffectType> LPETypeConverter(LPETypeData, INVALID_LPE);

Effect*
Effect::New(EffectType lpenr, LivePathEffectObject *lpeobj)
{
    Effect* neweffect = NULL;
    switch (lpenr) {
        case SKELETAL_STROKES:
            neweffect = (Effect*) new LPESkeletalStrokes(lpeobj);
            break;
        case PATH_ALONG_PATH:
            neweffect = (Effect*) new LPEPathAlongPath(lpeobj);
            break;
#ifdef LPE_ENABLE_TEST_EFFECTS
            case SLANT:
            neweffect = (Effect*) new LPESlant(lpeobj);
            break;
        case DOEFFECTSTACK_TEST:
            neweffect = (Effect*) new LPEdoEffectStackTest(lpeobj);
            break;
#endif
        case GEARS:
            neweffect = (Effect*) new LPEGears(lpeobj);
            break;
        case CURVE_STITCH:
            neweffect = (Effect*) new LPECurveStitch(lpeobj);
            break;
        default:
            g_warning("LivePathEffect::Effect::New   called with invalid patheffect type (%d)", lpenr);
            neweffect = NULL;
            break;
    }

    if (neweffect) {
        neweffect->readallParameters(SP_OBJECT_REPR(lpeobj));
    }

    return neweffect;
}

Effect::Effect(LivePathEffectObject *lpeobject)
{
    vbox = NULL;
    tooltips = NULL;
    lpeobj = lpeobject;
    oncanvasedit_it = 0;
}

Effect::~Effect()
{
    if (tooltips) {
        delete tooltips;
    }
}

Glib::ustring
Effect::getName()
{
    if (lpeobj->effecttype_set && lpeobj->effecttype < INVALID_LPE)
        return Glib::ustring( _(LPETypeConverter.get_label(lpeobj->effecttype).c_str()) );
    else
        return Glib::ustring( _("No effect") );
}

/*
 *  Here be the doEffect function chain:
 */
void
Effect::doEffect (SPCurve * curve)
{
    NArtBpath *new_bpath = doEffect_nartbpath(SP_CURVE_BPATH(curve));

    if (new_bpath && new_bpath != SP_CURVE_BPATH(curve)) {        // FIXME, add function to SPCurve to change bpath? or a copy function?
        if (curve->_bpath) {
            g_free(curve->_bpath); //delete old bpath
        }
        curve->_bpath = new_bpath;
    }
}

NArtBpath *
Effect::doEffect_nartbpath (NArtBpath * path_in)
{
    try {
        std::vector<Geom::Path> orig_pathv = BPath_to_2GeomPath(path_in);

        std::vector<Geom::Path> result_pathv = doEffect_path(orig_pathv);

        NArtBpath *new_bpath = BPath_from_2GeomPath(result_pathv);

        return new_bpath;
    }
    catch (std::exception & e) {
        g_warning("Exception during LPE %s execution. \n %s", getName().c_str(), e.what());
        SP_ACTIVE_DESKTOP->messageStack()->flash( Inkscape::WARNING_MESSAGE,
            _("An exception occurred during execution of the Path Effect.") );

        NArtBpath *path_out;

        unsigned ret = 0;
        while ( path_in[ret].code != NR_END ) {
            ++ret;
        }
        unsigned len = ++ret;

        path_out = g_new(NArtBpath, len);
        memcpy(path_out, path_in, len * sizeof(NArtBpath));
        return path_out;
    }
}

std::vector<Geom::Path>
Effect::doEffect_path (std::vector<Geom::Path> & path_in)
{
    Geom::Piecewise<Geom::D2<Geom::SBasis> > pwd2_in;

    for (unsigned int i=0; i < path_in.size(); i++) {
        pwd2_in.concat( path_in[i].toPwSb() );
    }

    Geom::Piecewise<Geom::D2<Geom::SBasis> > pwd2_out = doEffect_pwd2(pwd2_in);

    std::vector<Geom::Path> path_out = Geom::path_from_piecewise( pwd2_out, LPE_CONVERSION_TOLERANCE);

    return path_out;
}

Geom::Piecewise<Geom::D2<Geom::SBasis> >
Effect::doEffect_pwd2 (Geom::Piecewise<Geom::D2<Geom::SBasis> > & pwd2_in)
{
    g_warning("Effect has no doEffect implementation");
    return pwd2_in;
}

void
Effect::readallParameters(Inkscape::XML::Node * repr)
{
    std::vector<Parameter *>::iterator it = param_vector.begin();
    while (it != param_vector.end()) {
        Parameter * param = *it;
        const gchar * key = param->param_key.c_str();
        const gchar * value = repr->attribute(key);
        if (value) {
            bool accepted = param->param_readSVGValue(value);
            if (!accepted) { 
                g_warning("Effect::readallParameters - '%s' not accepted for %s", value, key);
            }
        } else {
            // set default value
            param->param_set_default();
        }

        it++;
    }
}

/* This function does not and SHOULD NOT write to XML */
void
Effect::setParameter(const gchar * key, const gchar * new_value)
{
    Parameter * param = getParameter(key);
    if (param) {
        if (new_value) {
            bool accepted = param->param_readSVGValue(new_value);
            if (!accepted) { 
                g_warning("Effect::setParameter - '%s' not accepted for %s", new_value, key);
            }
        } else {
            // set default value
            param->param_set_default();
        }
    }
}

void
Effect::registerParameter(Parameter * param)
{
    param_vector.push_back(param);
}

Gtk::Widget *
Effect::getWidget()
{
    if (!vbox) {
        vbox = Gtk::manage( new Gtk::VBox() ); // use manage here, because after deletion of Effect object, others might still be pointing to this widget.
        //if (!tooltips)
            tooltips = new Gtk::Tooltips();

        vbox->set_border_width(5);

        std::vector<Parameter *>::iterator it = param_vector.begin();
        while (it != param_vector.end()) {
            Parameter * param = *it;
            Gtk::Widget * widg = param->param_getWidget();
            Glib::ustring * tip = param->param_getTooltip();
            if (widg) {
               vbox->pack_start(*widg, true, true, 2);
                if (tip != NULL) {
                    tooltips->set_tip(*widg, *tip);
                }
            }

            it++;
        }
    }

    return dynamic_cast<Gtk::Widget *>(vbox);
}


Inkscape::XML::Node *
Effect::getRepr()
{
    return SP_OBJECT_REPR(lpeobj);
}

SPDocument *
Effect::getSPDoc()
{
    if (SP_OBJECT_DOCUMENT(lpeobj) == NULL) g_message("Effect::getSPDoc() returns NULL");
    return SP_OBJECT_DOCUMENT(lpeobj);
}

Parameter *
Effect::getParameter(const char * key)
{
    Glib::ustring stringkey(key);

    std::vector<Parameter *>::iterator it = param_vector.begin();
    while (it != param_vector.end()) {
        Parameter * param = *it;
        if ( param->param_key == key) {
            return param;
        }

        it++;
    }

    return NULL;
}

Parameter *
Effect::getNextOncanvasEditableParam()
{
    oncanvasedit_it++;
    if (oncanvasedit_it == static_cast<int>(param_vector.size())) {
        oncanvasedit_it = 0;
    }
    int old_it = oncanvasedit_it;

    do {
        Parameter * param = param_vector[oncanvasedit_it];
        if(param && param->oncanvas_editable) {
            return param;
        } else {
            oncanvasedit_it++;
            if (oncanvasedit_it == static_cast<int>(param_vector.size())) {  // loop round the map
                oncanvasedit_it = 0;
            }
        }
    } while (oncanvasedit_it != old_it); // iterate until complete loop through map has been made

    return NULL;
}

void
Effect::editNextParamOncanvas(SPItem * item, SPDesktop * desktop)
{
    if (!desktop) return;

    Parameter * param = getNextOncanvasEditableParam();
    if (param) {
        param->param_editOncanvas(item, desktop);
        gchar *message = g_strdup_printf(_("Editing parameter <b>%s</b>."), param->param_label.c_str());
        desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, message);
        g_free(message);
    } else {
        desktop->messageStack()->flash( Inkscape::WARNING_MESSAGE,
                                        _("None of the applied path effect's parameters can be edited on-canvas.") );
    }
}

/* This function should reset the defaults and is used for example to initialize an effect right after it has been applied to a path
* The nice thing about this is that this function can use knowledge of the original path and set things accordingly for example to the size or origin of the original path!
*/
void
Effect::resetDefaults(SPItem * /*item*/)
{
    // do nothing for simple effects
}

void
Effect::setup_nodepath(Inkscape::NodePath::Path *np)
{
    np->show_helperpath = true;
    np->helperpath_rgba = 0xff0000ff;
    np->helperpath_width = 1.0;
}

void
Effect::transform_multiply(Geom::Matrix const& postmul, bool set)
{
    // cycle through all parameters. Most parameters will not need transformation, but path and point params do.
    for (std::vector<Parameter *>::iterator it = param_vector.begin(); it != param_vector.end(); it++) {
        Parameter * param = *it;
        param->param_transform_multiply(postmul, set);
    }
}

} /* namespace LivePathEffect */

} /* namespace Inkscape */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
