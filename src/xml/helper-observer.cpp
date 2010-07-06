#include "helper-observer.h"

namespace Inkscape {
namespace XML {

// Very simple observer that just emits a signal if anything happens to a node
SignalObserver::SignalObserver()
    : _oldsel(0)
{}

// Add this observer to the SPObject and remove it from any previous object
void SignalObserver::set(SPObject* o)
{
	/* XML Tree being used direcly in this function in the following code
	   while it shouldn't be */
    if(_oldsel && _oldsel->getRepr())
        _oldsel->getRepr()->removeObserver(*this);
    if(o && o->getRepr())
        o->getRepr()->addObserver(*this);
    _oldsel = o;
}

void SignalObserver::notifyChildAdded(XML::Node&, XML::Node&, XML::Node*)
{ signal_changed()(); }

void SignalObserver::notifyChildRemoved(XML::Node&, XML::Node&, XML::Node*)
{ signal_changed()(); }

void SignalObserver::notifyChildOrderChanged(XML::Node&, XML::Node&, XML::Node*, XML::Node*)
{ signal_changed()(); }

void SignalObserver::notifyContentChanged(XML::Node&, Util::ptr_shared<char>, Util::ptr_shared<char>)
{}

void SignalObserver::notifyAttributeChanged(XML::Node&, GQuark, Util::ptr_shared<char>, Util::ptr_shared<char>)
{ signal_changed()(); }

sigc::signal<void>& SignalObserver::signal_changed()
{
    return _signal_changed;
}

} //namespace XML
} //namespace Inkscape

