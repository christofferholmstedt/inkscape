
/*
 * A simple interface for previewing representations.
 *
 * Authors:
 *   Jon A. Cruz
 *
 * Copyright (C) 2005 Jon A. Cruz
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */


#include "previewholder.h"
#include "preferences.h"

#include <gtkmm/scrolledwindow.h>
#include <gtkmm/sizegroup.h>
#include <gtkmm/scrollbar.h>
#include <gtkmm/adjustment.h>

#define COLUMNS_FOR_SMALL 16
#define COLUMNS_FOR_LARGE 8
//#define COLUMNS_FOR_SMALL 48
//#define COLUMNS_FOR_LARGE 32


namespace Inkscape {
namespace UI {


PreviewHolder::PreviewHolder() :
    VBox(),
    PreviewFillable(),
    _scroller(0),
    _insides(0),
    _prefCols(0),
    _updatesFrozen(false),
    _anchor(SP_ANCHOR_CENTER),
    _baseSize(PREVIEW_SIZE_SMALL),
    _ratio(100),
    _view(VIEW_TYPE_LIST),
    _wrap(false),
    _border(BORDER_NONE)
{
    _scroller = manage(new Gtk::ScrolledWindow());
    _insides = manage(new Gtk::Table( 1, 2 ));
    _insides->set_col_spacings( 8 );

    // Add a container with the scroller and a spacer
    Gtk::Table* spaceHolder = manage( new Gtk::Table(1, 2) );
    _scroller->add( *_insides );
    spaceHolder->attach( *_scroller, 0, 1, 0, 1, Gtk::FILL|Gtk::EXPAND, Gtk::FILL|Gtk::EXPAND );

    pack_start(*spaceHolder, Gtk::PACK_EXPAND_WIDGET);
}

PreviewHolder::~PreviewHolder()
{

}

bool PreviewHolder::on_scroll_event(GdkEventScroll *event)
{
    // Scroll horizontally by page on mouse wheel
    Gtk::Adjustment *adj = dynamic_cast<Gtk::ScrolledWindow*>(_scroller)->get_hadjustment();
    if (!adj) {
        return FALSE;
    }

    int move = (event->direction == GDK_SCROLL_DOWN) ? adj->get_page_size() : -adj->get_page_size();

    double value = std::min(adj->get_upper() - move, adj->get_value() + move );

    adj->set_value(value);

    return FALSE;
}

void PreviewHolder::clear()
{
    items.clear();
    _prefCols = 0;
    // Kludge to restore scrollbars
    if ( !_wrap && (_view != VIEW_TYPE_LIST) && (_anchor == SP_ANCHOR_NORTH || _anchor == SP_ANCHOR_SOUTH) ) {
        dynamic_cast<Gtk::ScrolledWindow*>(_scroller)->set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_NEVER );
    }
    rebuildUI();
}

void PreviewHolder::addPreview( Previewable* preview )
{
    items.push_back(preview);
    if ( !_updatesFrozen )
    {
        int i = items.size() - 1;

        if ( _view == VIEW_TYPE_LIST ) {
            Gtk::Widget* label = manage(preview->getPreview(PREVIEW_STYLE_BLURB, VIEW_TYPE_LIST, _baseSize, _ratio, _border));
            Gtk::Widget* thing = manage(preview->getPreview(PREVIEW_STYLE_PREVIEW, VIEW_TYPE_LIST, _baseSize, _ratio, _border));

            _insides->attach( *thing, 0, 1, i, i+1, Gtk::FILL|Gtk::EXPAND, Gtk::FILL|Gtk::EXPAND );
            _insides->attach( *label, 1, 2, i, i+1, Gtk::FILL|Gtk::EXPAND, Gtk::SHRINK );
        } else {
            Gtk::Widget* thing = manage(items[i]->getPreview(PREVIEW_STYLE_PREVIEW, VIEW_TYPE_GRID, _baseSize, _ratio, _border));

            int width = 1;
            int height = 1;
            calcGridSize( thing, items.size(), width, height );
            int col = i % width;
            int row = i / width;

            if ( _insides && width > (int)_insides->property_n_columns() ) {
                std::vector<Gtk::Widget*>kids = _insides->get_children();
                int oldWidth = (int)_insides->property_n_columns();
                int childCount = (int)kids.size();
//             g_message("  %3d  resize from %d to %d  (r:%d, c:%d)  with %d children", i, oldWidth, width, row, col, childCount );
                _insides->resize( height, width );

                for ( int j = oldWidth; j < childCount; j++ ) {
                    Gtk::Widget* target = kids[childCount - (j + 1)];
                    int col2 = j % width;
                    int row2 = j / width;
                    Glib::RefPtr<Gtk::Widget> handle(target);
                    _insides->remove( *target );
                    _insides->attach( *target, col2, col2+1, row2, row2+1, Gtk::FILL|Gtk::EXPAND, Gtk::FILL|Gtk::EXPAND );
                }
            } else if ( col == 0 ) {
                // we just started a new row
                _insides->resize( row + 1, width );
            }
            _insides->attach( *thing, col, col+1, row, row+1, Gtk::FILL|Gtk::EXPAND, Gtk::FILL|Gtk::EXPAND );
        }

        _scroller->show_all_children();
        _scroller->queue_draw();
    }
}

void PreviewHolder::freezeUpdates()
{
    _updatesFrozen = true;
}

void PreviewHolder::thawUpdates()
{
    _updatesFrozen = false;
    rebuildUI();
}

void PreviewHolder::setStyle( ::PreviewSize size, ViewType view, guint ratio, ::BorderStyle border )
{
    if ( size != _baseSize || view != _view || ratio != _ratio || border != _border ) {
        _baseSize = size;
        _view = view;
        _ratio = ratio;
        _border = border;
        // Kludge to restore scrollbars
        if ( !_wrap && (_view != VIEW_TYPE_LIST) && (_anchor == SP_ANCHOR_NORTH || _anchor == SP_ANCHOR_SOUTH) ) {
            dynamic_cast<Gtk::ScrolledWindow*>(_scroller)->set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_NEVER );
        }
        rebuildUI();
    }
}

void PreviewHolder::setOrientation(SPAnchorType how)
{
    if ( _anchor != how )
    {
        _anchor = how;
        switch ( _anchor )
        {
            case SP_ANCHOR_NORTH:
            case SP_ANCHOR_SOUTH:
            {
                dynamic_cast<Gtk::ScrolledWindow*>(_scroller)->set_policy( Gtk::POLICY_AUTOMATIC, _wrap ? Gtk::POLICY_AUTOMATIC : Gtk::POLICY_NEVER );
            }
            break;

            case SP_ANCHOR_EAST:
            case SP_ANCHOR_WEST:
            {
                dynamic_cast<Gtk::ScrolledWindow*>(_scroller)->set_policy( Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC );
            }
            break;

            default:
            {
                dynamic_cast<Gtk::ScrolledWindow*>(_scroller)->set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC );
            }
        }
        rebuildUI();
    }
}

void PreviewHolder::setWrap( bool b )
{
    if ( b != _wrap ) {
        _wrap = b;
        switch ( _anchor )
        {
            case SP_ANCHOR_NORTH:
            case SP_ANCHOR_SOUTH:
            {
                dynamic_cast<Gtk::ScrolledWindow*>(_scroller)->set_policy( Gtk::POLICY_AUTOMATIC, _wrap ? Gtk::POLICY_AUTOMATIC : Gtk::POLICY_NEVER );
            }
            break;
            default:
            {
                (void)0;
                // do nothing;
            }
        }
        rebuildUI();
    }
}

void PreviewHolder::setColumnPref( int cols )
{
    _prefCols = cols;
}

void PreviewHolder::on_size_allocate( Gtk::Allocation& allocation )
{
//     g_message( "on_size_allocate(%d, %d) (%d, %d)", allocation.get_x(), allocation.get_y(), allocation.get_width(), allocation.get_height() );
//     g_message("            anchor:%d", _anchor);
    Gtk::VBox::on_size_allocate( allocation );

    if ( _insides && !_wrap && (_view != VIEW_TYPE_LIST) && (_anchor == SP_ANCHOR_NORTH || _anchor == SP_ANCHOR_SOUTH) ) {
	Gtk::Requisition req;
#if GTK_CHECK_VERSION(3,0,0)
	Gtk::Requisition req_natural;
	_insides->get_preferred_size(req, req_natural);
#else
        req = _insides->size_request();
#endif
        gint delta = allocation.get_width() - req.width;

        if ( (delta > 4) && req.height < allocation.get_height() ) {
            dynamic_cast<Gtk::ScrolledWindow*>(_scroller)->set_policy( Gtk::POLICY_NEVER, Gtk::POLICY_NEVER );
        } else {
            dynamic_cast<Gtk::ScrolledWindow*>(_scroller)->set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_NEVER );
        }
    }
}

//void PreviewHolder::on_size_request( Gtk::Requisition* requisition )
//{
//     g_message( "on_size_request(%d, %d)", requisition->width, requisition->height );
//    Gtk::VBox::on_size_request( requisition );
//     g_message( "   super       (%d, %d)", requisition->width, requisition->height );
//     g_message("            anchor:%d", _anchor);
//     g_message("             items:%d", (int)items.size());
//}

void PreviewHolder::calcGridSize( const Gtk::Widget* thing, int itemCount, int& width, int& height )
{
    width = itemCount;
    height = 1;

    if ( _anchor == SP_ANCHOR_SOUTH || _anchor == SP_ANCHOR_NORTH ) {
        Gtk::Requisition req;
#if GTK_CHECK_VERSION(3,0,0)
	Gtk::Requisition req_natural;
	_scroller->get_preferred_size(req, req_natural);
#else
       	req = _scroller->size_request();
#endif
        int currW = _scroller->get_width();
        if ( currW > req.width ) {
            req.width = currW;
        }

#if GTK_CHECK_VERSION(3,0,0)
        Gtk::Scrollbar* hs = dynamic_cast<Gtk::ScrolledWindow*>(_scroller)->get_hscrollbar();
#else
        Gtk::HScrollbar* hs = dynamic_cast<Gtk::ScrolledWindow*>(_scroller)->get_hscrollbar();
#endif

        if ( hs ) {
            Gtk::Requisition scrollReq;
#if GTK_CHECK_VERSION(3,0,0)
            Gtk::Requisition scrollReq_natural;
	    hs->get_preferred_size(scrollReq, scrollReq_natural);
#else
	    scrollReq = hs->size_request();
#endif

            // the +8 is a temporary hack
            req.height -= scrollReq.height + 8;
        }

        Gtk::Requisition req2;
#if GTK_CHECK_VERSION(3,0,0)
        Gtk::Requisition req2_natural;
	const_cast<Gtk::Widget*>(thing)->get_preferred_size(req2, req2_natural);
#else
       	req2 = const_cast<Gtk::Widget*>(thing)->size_request();
#endif

        int h2 = ((req2.height > 0) && (req.height > req2.height)) ? (req.height / req2.height) : 1;
        int w2 = ((req2.width > 0) && (req.width > req2.width)) ? (req.width / req2.width) : 1;
        width = (itemCount + (h2 - 1)) / h2;
        if ( width < w2 ) {
            width = w2;
        }
    } else {
        width = (_baseSize == PREVIEW_SIZE_SMALL || _baseSize == PREVIEW_SIZE_TINY) ? COLUMNS_FOR_SMALL : COLUMNS_FOR_LARGE;
        if ( _prefCols > 0 ) {
            width = _prefCols;
        }
        height = (itemCount + (width - 1)) / width;
        if ( height < 1 ) {
            height = 1;
        }
    }
}

void PreviewHolder::rebuildUI()
{
    _scroller->remove();
    _insides = 0; // remove() call should have deleted the Gtk::Table.

    if ( _view == VIEW_TYPE_LIST ) {
        _insides = manage(new Gtk::Table( 1, 2 ));
        _insides->set_col_spacings( 8 );
        if (_border == BORDER_WIDE) {
            _insides->set_row_spacings( 1 );
        }

        for ( unsigned int i = 0; i < items.size(); i++ ) {
            Gtk::Widget* label = manage(items[i]->getPreview(PREVIEW_STYLE_BLURB, _view, _baseSize, _ratio, _border));
            //label->set_alignment(Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER);

            Gtk::Widget* thing = manage(items[i]->getPreview(PREVIEW_STYLE_PREVIEW, _view, _baseSize, _ratio, _border));

            _insides->attach( *thing, 0, 1, i, i+1, Gtk::FILL|Gtk::EXPAND, Gtk::FILL|Gtk::EXPAND );
            _insides->attach( *label, 1, 2, i, i+1, Gtk::FILL|Gtk::EXPAND, Gtk::SHRINK );
        }
        _scroller->add( *_insides );
    } else {
        int col = 0;
        int row = 0;
        int width = 2;
        int height = 1;

        for ( unsigned int i = 0; i < items.size(); i++ ) {

            // If this is the last row, flag so the previews can draw a bottom
            ::BorderStyle border = ((row == height -1) && (_border == BORDER_SOLID)) ? BORDER_SOLID_LAST_ROW : _border;
            Gtk::Widget* thing = manage(items[i]->getPreview(PREVIEW_STYLE_PREVIEW, _view, _baseSize, _ratio, border));

            if ( !_insides ) {
                calcGridSize( thing, items.size(), width, height );
                _insides = manage(new Gtk::Table( height, width ));
                if (_border == BORDER_WIDE) {
                    _insides->set_col_spacings( 1 );
                    _insides->set_row_spacings( 1 );
                }
            }

            _insides->attach( *thing, col, col+1, row, row+1, Gtk::FILL|Gtk::EXPAND, Gtk::FILL|Gtk::EXPAND );
            if ( ++col >= width ) {
                col = 0;
                row++;
            }
        }
        if ( !_insides ) {
            _insides = manage(new Gtk::Table( 1, 2 ));
        }

        _scroller->add( *_insides );
    }

    _scroller->show_all_children();
    _scroller->queue_draw();
}





} //namespace UI
} //namespace Inkscape


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
