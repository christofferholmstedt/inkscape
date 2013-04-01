#ifndef SEEN_SP_ITEM_FLOWTEXT_H
#define SEEN_SP_ITEM_FLOWTEXT_H

/*
 */

#include "sp-item.h"

#include <2geom/forward.h>
#include "libnrtype/Layout-TNG.h"

#define SP_TYPE_FLOWTEXT            (sp_flowtext_get_type ())
#define SP_FLOWTEXT(obj) ((SPFlowtext*)obj)
#define SP_IS_FLOWTEXT(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPFlowtext)))


namespace Inkscape {

class DrawingGroup;

} // namespace Inkscape

class CFlowtext;

class SPFlowtext : public SPItem {
public:
	SPFlowtext();
	CFlowtext* cflowtext;

    /** Completely recalculates the layout. */
    void rebuildLayout();

    /** Converts the flowroot in into a \<text\> tree, keeping all the formatting and positioning,
    but losing the automatic wrapping ability. */
    Inkscape::XML::Node *getAsText();

    SPItem *get_frame(SPItem *after);

    bool has_internal_frame();

//semiprivate:  (need to be accessed by the C-style functions still)
    Inkscape::Text::Layout layout;

    /** discards the drawing objects representing this text. */
    void _clearFlow(Inkscape::DrawingGroup* in_arena);

    double par_indent;

private:
    /** Recursively walks the xml tree adding tags and their contents. */
    void _buildLayoutInput(SPObject *root, Shape const *exclusion_shape, std::list<Shape> *shapes, SPObject **pending_line_break_object);

    /** calculates the union of all the \<flowregionexclude\> children
    of this flowroot. */
    Shape* _buildExclusionShape() const;

};

struct SPFlowtextClass {
    SPItemClass parent_class;
};

class CFlowtext : public CItem {
public:
	CFlowtext(SPFlowtext* flowtext);
	virtual ~CFlowtext();

	virtual void build(SPDocument* doc, Inkscape::XML::Node* repr);

	virtual void child_added(Inkscape::XML::Node* child, Inkscape::XML::Node* ref);
	virtual void remove_child(Inkscape::XML::Node* child);

	virtual void set(unsigned int key, const gchar* value);

	virtual void update(SPCtx* ctx, unsigned int flags);
	virtual void modified(unsigned int flags);

	virtual Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

	virtual Geom::OptRect bbox(Geom::Affine const &transform, SPItem::BBoxType type);
	virtual void print(SPPrintContext *ctx);
	virtual gchar* description();
	virtual Inkscape::DrawingItem* show(Inkscape::Drawing &drawing, unsigned int key, unsigned int flags);
	virtual void hide(unsigned int key);
    virtual void snappoints(std::vector<Inkscape::SnapCandidatePoint> &p, Inkscape::SnapPreferences const *snapprefs);
protected:
	SPFlowtext* spflowtext;
};


GType sp_flowtext_get_type (void);

SPItem *create_flowtext_with_internal_frame (SPDesktop *desktop, Geom::Point p1, Geom::Point p2);

#endif // SEEN_SP_ITEM_FLOWTEXT_H

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
