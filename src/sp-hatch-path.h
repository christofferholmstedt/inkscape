/** @file
 * SVG <hatchPath> implementation
 *//*
 * Author:
 *   Tomasz Boczkowski <penginsbacon@gmail.com>
 *
 * Copyright (C) 2014 Tomasz Boczkowski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SEEN_SP_HATCH_PATH_H
#define SEEN_SP_HATCH_PATH_H

#include <list>
#include <stddef.h>
#include <glibmm/ustring.h>
#include <sigc++/connection.h>

#include "svg/svg-length.h"

namespace Inkscape {

class Drawing;
class DrawingShape;

}

#define SP_HATCH_PATH(obj) (dynamic_cast<SPHatchPath*>((SPObject*)obj))
#define SP_IS_HATCH_PATH(obj) (dynamic_cast<const SPHatchPath*>((SPObject*)obj) != NULL)

class SPHatchPath : public SPObject {
public:
    SPHatchPath();
	virtual ~SPHatchPath();

	SVGLength offset;

	void setCurve(SPCurve *curve, bool owner);

    bool isValid() const;

    Inkscape::DrawingItem *show(Inkscape::Drawing &drawing, unsigned int key);
    void hide(unsigned int key);

	void setStripExtents(unsigned int key, Geom::OptInterval const &extents);

protected:
	virtual void build(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void release();
	virtual void set(unsigned int key, const gchar* value);
	virtual void update(SPCtx* ctx, unsigned int flags);

private:
    struct View {
        View(Inkscape::DrawingShape *arenaitem, int key);
        //Do not delete arenaitem in destructor.

        Inkscape::DrawingShape *arenaitem;
        Geom::OptInterval extents;
        unsigned int key;
    };
    typedef std::list<SPHatchPath::View>::iterator ViewIterator;
    std::list<View> _display;

    gdouble _repeatLength() const;
    void _updateView(View &view);

    void _readHatchPathVector(char const *str, Geom::PathVector &pathv, bool &continous_join);

    SPCurve *_curve;
    bool _continuous;
};

#endif // SEEN_SP_HATCH_PATH_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
