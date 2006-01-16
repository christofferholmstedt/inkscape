/**
 * Phoebe DOM Implementation.
 *
 * This is a C++ approximation of the W3C DOM model, which follows
 * fairly closely the specifications in the various .idl files, copies of
 * which are provided for reference.  Most important is this one:
 *
 * http://www.w3.org/TR/2004/REC-DOM-Level-3-Core-20040407/idl-definitions.html
 *
 * Authors:
 *   Bob Jamison
 *
 * Copyright (C) 2005 Bob Jamison
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "svgimpl.h"



namespace org
{
namespace w3c
{
namespace dom
{
namespace svg
{


/*#########################################################################
## SVGElementImpl
#########################################################################*/


//##################
//# Non-API methods
//##################




/*#########################################################################
## SVGDocumentImpl
#########################################################################*/



//####################################################
//# Overload some createXXX() methods from DocumentImpl,
//# To create our SVG-DOM types
//####################################################

/**
 *
 */
Element *SVGDocumentImpl::createElement(const DOMString& tagName)
                           throw(DOMException)
{
    SVGElementImpl *impl = new SVGElementImpl(this, tagName);
    return impl;
}


/**
 *
 */
Element *SVGDocumentImpl::createElementNS(const DOMString& namespaceURI,
                             const DOMString& qualifiedName)
                             throw(DOMException)
{
    SVGElementImpl *elem = new SVGElementImpl(this, namespaceURI, qualifiedName);
    return elem;
}



//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGSVGElementImpl
#########################################################################*/


/**
 *
 */
unsigned long SVGSVGElementImpl::suspendRedraw(unsigned long max_wait_milliseconds )
{
    return 0L;
}

/**
 *
 */
void SVGSVGElementImpl::unsuspendRedraw(unsigned long suspend_handle_id )
                                  throw ( DOMException )
{
}


/**
 *
 */
void SVGSVGElementImpl::unsuspendRedrawAll(  )
{
}

/**
 *
 */
void SVGSVGElementImpl::forceRedraw(  )
{
}

/**
 *
 */
void SVGSVGElementImpl::pauseAnimations(  )
{
}

/**
 *
 */
void SVGSVGElementImpl::unpauseAnimations(  )
{
}

/**
 *
 */
bool SVGSVGElementImpl::animationsPaused(  )
{
    return false;
}


/**
 *
 */
NodeList SVGSVGElementImpl::getIntersectionList(const SVGRect &rect,
                                           const SVGElement *referenceElement )
{
    NodeList list;
    return list;
}

/**
 *
 */
NodeList SVGSVGElementImpl::getEnclosureList(const SVGRect &rect,
                                        const SVGElement *referenceElement )
{
    NodeList list;
    return list;
}

/**
 *
 */
bool SVGSVGElementImpl::checkIntersection(const SVGElement *element,
                                          const SVGRect &rect )
{
    return false;
}

/**
 *
 */
bool SVGSVGElementImpl::checkEnclosure(const SVGElement *element,
                                       const SVGRect &rect )
{
    return false;
}

/**
 *
 */
void SVGSVGElementImpl::deselectAll(  )
{
}

/**
 *
 */
Element *SVGSVGElementImpl::getElementById(const DOMString& elementId )
{
    return NULL;
}



//##################
//# Non-API methods
//##################





/*#########################################################################
## SVGGElementImpl
#########################################################################*/


//##################
//# Non-API methods
//##################






/*#########################################################################
## SVGDefsElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################






/*#########################################################################
## SVGDescElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################






/*#########################################################################
## SVGTitleElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################




/*#########################################################################
## SVGSymbolElementImpl
#########################################################################*/




//##################
//# Non-API methods
//##################






/*#########################################################################
## SVGUseElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################









/*#########################################################################
## SVGImageElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################








/*#########################################################################
## SVGSwitchElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################






/*#########################################################################
## GetSVGDocumentImpl
#########################################################################*/

/**
 *
 */
SVGDocument *GetSVGDocumentImpl::getSVGDocument(  )
                    throw ( DOMException )
{
    return NULL;
}

//##################
//# Non-API methods
//##################








/*#########################################################################
## SVGStyleElementImpl
#########################################################################*/





//##################
//# Non-API methods
//##################




/*#########################################################################
## SVGPathElementImpl
#########################################################################*/

/**
 *
 */
SVGAnimatedNumber SVGPathElementImpl::getPathLength()
{
    SVGAnimatedNumber ret;
    return ret;
}

/**
 *
 */
double SVGPathElementImpl::getTotalLength(  )
{
    return 0.0;
}

/**
 *
 */
SVGPoint SVGPathElementImpl::getPointAtLength(double distance )
{
    SVGPoint ret;
    return ret;
}

/**
 *
 */
unsigned long SVGPathElementImpl::getPathSegAtLength(double distance )
{
    return 0L;
}


//##################
//# Non-API methods
//##################








/*#########################################################################
## SVGRectElementImpl
#########################################################################*/





//##################
//# Non-API methods
//##################








/*#########################################################################
## SVGCircleElementImpl
#########################################################################*/




//##################
//# Non-API methods
//##################








/*#########################################################################
## SVGEllipseElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################








/*#########################################################################
## SVGLineElementImpl
#########################################################################*/


//##################
//# Non-API methods
//##################








/*#########################################################################
## SVGPolylineElementImpl
#########################################################################*/





//##################
//# Non-API methods
//##################






/*#########################################################################
## SVGPolygonElementImpl
#########################################################################*/






//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGTextContentElementImpl
#########################################################################*/

/**
 *
 */
SVGAnimatedLength SVGTextContentElementImpl::getTextLength()
{
    SVGAnimatedLength ret;
    return ret;
}


/**
 *
 */
SVGAnimatedEnumeration SVGTextContentElementImpl::getLengthAdjust()
{
    SVGAnimatedEnumeration ret;
    return ret;
}


/**
 *
 */
long SVGTextContentElementImpl::getNumberOfChars(  )
{
    return 0L;
}

/**
 *
 */
double SVGTextContentElementImpl::getComputedTextLength(  )
{
    return 0.0;
}

/**
 *
 */
double SVGTextContentElementImpl::getSubStringLength(unsigned long charnum, unsigned long nchars )
                                     throw ( DOMException )
{
    return 0.0;
}

/**
 *
 */
SVGPoint SVGTextContentElementImpl::getStartPositionOfChar(unsigned long charnum )
                                              throw ( DOMException )
{
    SVGPoint ret;
    return ret;
}

/**
 *
 */
SVGPoint SVGTextContentElementImpl::getEndPositionOfChar(unsigned long charnum )
                                           throw ( DOMException )
{
    SVGPoint ret;
    return ret;
}

/**
 *
 */
SVGRect SVGTextContentElementImpl::getExtentOfChar(unsigned long charnum )
                                      throw ( DOMException )
{
    SVGRect ret;
    return ret;
}

/**
 *
 */
double SVGTextContentElementImpl::getRotationOfChar(unsigned long charnum )
                                     throw ( DOMException )
{
    return 0.0;
}

/**
 *
 */
long SVGTextContentElementImpl::getCharNumAtPosition(const SVGPoint &point )
{
    return 0L;
}

/**
 *
 */
void SVGTextContentElementImpl::selectSubString(unsigned long charnum,
                                                unsigned long nchars )
                                                throw ( DOMException )
{
}



//##################
//# Non-API methods
//##################









/*#########################################################################
## SVGTextPositioningElementImpl
#########################################################################*/




//##################
//# Non-API methods
//##################








/*#########################################################################
## SVGTextElementImpl
#########################################################################*/





//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGTSpanElementImpl
#########################################################################*/





//##################
//# Non-API methods
//##################






/*#########################################################################
## SVGTRefElementImpl
#########################################################################*/





//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGTextPathElementImpl
#########################################################################*/




//##################
//# Non-API methods
//##################









/*#########################################################################
## SVGAltGlyphElementImpl
#########################################################################*/






//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGAltGlyphDefElementImpl
#########################################################################*/





//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGAltGlyphItemElementImpl
#########################################################################*/





//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGGlyphRefElementImpl
#########################################################################*/




//##################
//# Non-API methods
//##################








/*#########################################################################
## SVGMarkerElementImpl
#########################################################################*/




//##################
//# Non-API methods
//##################








/*#########################################################################
## SVGColorProfileElementImpl
#########################################################################*/




//##################
//# Non-API methods
//##################









/*#########################################################################
## SVGGradientElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGLinearGradientElementImpl
#########################################################################*/




//##################
//# Non-API methods
//##################








/*#########################################################################
## SVGRadialGradientElementImpl
#########################################################################*/





//##################
//# Non-API methods
//##################








/*#########################################################################
## SVGStopElementImpl
#########################################################################*/





//##################
//# Non-API methods
//##################









/*#########################################################################
## SVGPatternElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################








/*#########################################################################
## SVGClipPathElementImpl
#########################################################################*/






//##################
//# Non-API methods
//##################









/*#########################################################################
## SVGMaskElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################









/*#########################################################################
## SVGFilterElementImpl
#########################################################################*/





//##################
//# Non-API methods
//##################









/*#########################################################################
## SVGFEBlendElementImpl
#########################################################################*/




//##################
//# Non-API methods
//##################








/*#########################################################################
## SVGFEColorMatrixElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################









/*#########################################################################
## SVGFEComponentTransferElementImpl
#########################################################################*/




//##################
//# Non-API methods
//##################










/*#########################################################################
## SVGComponentTransferFunctionElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################









/*#########################################################################
## SVGFEFuncRElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGFEFuncGElementImpl
#########################################################################*/





//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGFEFuncBElementImpl
#########################################################################*/





//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGFEFuncAElementImpl
#########################################################################*/





//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGFECompositeElementImpl
#########################################################################*/




//##################
//# Non-API methods
//##################








/*#########################################################################
## SVGFEConvolveMatrixElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################








/*#########################################################################
## SVGFEDiffuseLightingElementImpl
#########################################################################*/




//##################
//# Non-API methods
//##################





/*#########################################################################
## SVGFEDistantLightElementImpl
#########################################################################*/





//##################
//# Non-API methods
//##################









/*#########################################################################
## SVGFEPointLightElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################









/*#########################################################################
## SVGFESpotLightElementImpl
#########################################################################*/






//##################
//# Non-API methods
//##################









/*#########################################################################
## SVGFEDisplacementMapElementImpl
#########################################################################*/




//##################
//# Non-API methods
//##################









/*#########################################################################
## SVGFEFloodElementImpl
#########################################################################*/





//##################
//# Non-API methods
//##################









/*#########################################################################
## SVGFEGaussianBlurElementImpl
#########################################################################*/





//##################
//# Non-API methods
//##################









/*#########################################################################
## SVGFEImageElementImpl
#########################################################################*/




//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGFEMergeElementImpl
#########################################################################*/




//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGFEMergeNodeElementImpl
#########################################################################*/





//##################
//# Non-API methods
//##################








/*#########################################################################
## SVGFEMorphologyElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################








/*#########################################################################
## SVGFEOffsetElementImpl
#########################################################################*/




//##################
//# Non-API methods
//##################






/*#########################################################################
## SVGFESpecularLightingElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################









/*#########################################################################
## SVGFETileElementImpl
#########################################################################*/





//##################
//# Non-API methods
//##################









/*#########################################################################
## SVGFETurbulenceElementImpl
#########################################################################*/






//##################
//# Non-API methods
//##################









/*#########################################################################
## SVGCursorElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################









/*#########################################################################
## SVGAElementImpl
#########################################################################*/





//##################
//# Non-API methods
//##################








/*#########################################################################
## SVGViewElementImpl
#########################################################################*/






//##################
//# Non-API methods
//##################








/*#########################################################################
## SVGScriptElementImpl
#########################################################################*/




//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGAnimationElementImpl
#########################################################################*/





//##################
//# Non-API methods
//##################









/*#########################################################################
## SVGAnimateElementImpl
#########################################################################*/


//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGSetElementImpl
#########################################################################*/




//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGAnimateMotionElementImpl
#########################################################################*/


//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGMPathElementImpl
#########################################################################*/


//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGAnimateColorElementImpl
#########################################################################*/


//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGAnimateTransformElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGFontElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGGlyphElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGMissingGlyphElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGHKernElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGVKernElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGFontFaceElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGFontFaceSrcElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGFontFaceUriElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################





/*#########################################################################
## SVGFontFaceFormatElementImpl
#########################################################################*/




//##################
//# Non-API methods
//##################






/*#########################################################################
## SVGFontFaceNameElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGDefinitionSrcElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################







/*#########################################################################
## SVGMetadataElementImpl
#########################################################################*/


//##################
//# Non-API methods
//##################





/*#########################################################################
## SVGForeignObjectElementImpl
#########################################################################*/



//##################
//# Non-API methods
//##################












}  //namespace svg
}  //namespace dom
}  //namespace w3c
}  //namespace org


/*#########################################################################
## E N D    O F    F I L E
#########################################################################*/

