/** @file
 * @brief New node tool with support for multiple path editing
 */
/* Authors:
 *   Krzysztof Kosiński <tweenk@gmail.com>
 *
 * Copyright (C) 2009 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SEEN_UI_TOOL_NODE_TOOL_H
#define SEEN_UI_TOOL_NODE_TOOL_H

#include <boost/ptr_container/ptr_map.hpp>
#include <glib.h>
#include "event-context.h"

namespace Inkscape {
	namespace Display {
		class TemporaryItem;
	}

	namespace UI {
		class MultiPathManipulator;
		class ControlPointSelection;
		class Selector;
		class ControlPoint;

		struct PathSharedData;
	}
}

#define INK_NODE_TOOL(obj) ((InkNodeTool*)obj)
#define INK_IS_NODE_TOOL(obj) (dynamic_cast<const InkNodeTool*>((const SPEventContext*)obj))

class InkNodeTool : public SPEventContext {
public:
	InkNodeTool();
	virtual ~InkNodeTool();

	Inkscape::UI::ControlPointSelection* _selected_nodes;
    Inkscape::UI::MultiPathManipulator* _multipath;

    bool edit_clipping_paths;
    bool edit_masks;

	static const std::string prefsPath;

	virtual void setup();
	virtual void set(const Inkscape::Preferences::Entry& val);
	virtual bool root_handler(GdkEvent* event);
	virtual bool item_handler(SPItem* item, GdkEvent* event);

	virtual const std::string& getPrefsPath();

private:
	sigc::connection _selection_changed_connection;
    sigc::connection _mouseover_changed_connection;
    sigc::connection _sizeUpdatedConn;

    Inkscape::MessageContext *_node_message_context;
    SPItem *flashed_item;
    Inkscape::Display::TemporaryItem *flash_tempitem;
    Inkscape::UI::Selector* _selector;
    Inkscape::UI::PathSharedData* _path_data;
    SPCanvasGroup *_transform_handle_group;
    SPItem *_last_over;
    boost::ptr_map<SPItem*, ShapeEditor> _shape_editors;

    bool cursor_drag;
    bool show_handles;
    bool show_outline;
    bool live_outline;
    bool live_objects;
    bool show_path_direction;
    bool show_transform_handles;
    bool single_node_transform_handles;

	void selection_changed(Inkscape::Selection *sel);

	void select_area(Geom::Rect const &sel, GdkEventButton *event);
	void select_point(Geom::Point const &sel, GdkEventButton *event);
	void mouseover_changed(Inkscape::UI::ControlPoint *p);
	void update_tip(GdkEvent *event);
	void handleControlUiStyleChange();
};

#endif

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
