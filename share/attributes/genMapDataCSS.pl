#!/usr/bin/env perl

# Purpose: To extract the css_defaults and cssprops data from the
# SVG 1.1 2nd Ed specification file propidx.html. The file can be found at:
#
#   http://www.w3.org/TR/SVG/propidx.html
#
# A number of adjustments must be made after parsing the HTML file.

# Author: Tavmjong Bah
# Rewrite of script by Abhishek

use strict;
use warnings;
use HTML::TokeParser;

# Groups of elements defined in spec.
# Note "use" is not a container element but it acts like one!
# Note "flowRoot, flowPara, flowSpan, flowRegion, and flowRect are Inkscape
#   specific (failed SVG1.2 items)
my @container_elements = ("a", "defs", "glyph", "g", "marker", "mask", "missing-glyph", "pattern", "svg", "switch", "symbol", "use");
my @graphics_elements = ("circle", "ellipse", "image", "line", "path", "polygon", "polyline", "rect", "text", "use");
my @filter_primitives = ("feBlend", "feColorMatrix", "feComponentTransfer", "feComposite", "feConvolveMatrix",
			 "feDiffuseLighting", "feDisplacementMap", "feFlood", "feGaussianBlur", "feImage",
			 "feMerge", "feMorphology", "feOffset", "feSpecularLighting", "feTile", "feTurbulence" );
my @text_content_elements = ("altGlyph", "textPath", "text", "tref", "tspan", "flowRoot", "flowPara", "flowSpan");
my @shapes = ("path", "rect", "circle", "ellipse", "line", "polyline", "polygon");
my @viewport = ("svg", "symbol", "foreignObject" );

my $p= HTML::TokeParser->new('propidx.html') or die "Can't open file: $!";

my %properties;
my $property;

# Loop over tokens
while( my $t = $p->get_token ) {

    # Look for <tr> (start token with value 'tr').
    if( $t->[0] eq 'S' and lc $t->[1] eq 'tr') {

	print "---------\n";

	my $column = 0;
	while( $t = $p->get_token ) {

	    # Keep track of column
	    if( $t->[0] eq 'S' and lc $t->[1] eq 'td') {
		$column++;
		$t = $p->get_token; # Skip to next token
	    }

	    if( $column == 1 and $t->[0] eq 'S' and lc $t->[1] =~ 'span') {
		# First column is always property name, defined inside <span>.
		# print "      span ${$t->[2]}{'class'}\n";
		# if( ${$t->[2]}{'class'} =~ 'prop-name' ) {
		#     print "        $t->[4]\n";
		# }		$property =~ s/\‘//;

		$t = $p->get_token;
		$property = $t->[1];
		$property =~ s/‘//; # Opening single quote
		$property =~ s/’//; # Closing single quote
		print "Property: $property\n";
	    }

	    if( $column == 3 and $t->[0] eq 'T') {
		# Third column is default value (simple text).
		my $default = $t->[1];
		$properties{ $property }->{default} = $default;
		print "  Default: $default\n";
	    }

	    if( $column == 4 and $t->[0] eq 'S' ) {
		# Fourth column is "Applies to"
		if( lc $t->[1] eq 'span' && ${$t->[2]}{'class'} =~ 'element-name' ) {
		    $t = $p->get_token;
		    my $element = $t->[1];
		    $element =~ s/‘//; # Opening single quote
		    $element =~ s/’//; # Closing single quote
		    print "  Elements: $element\n";
		    push @{$properties{ $property }->{elements}}, $element;
		} elsif ( lc $t->[1] eq 'a' ) {
		    my $text = $p->get_trimmed_text;
		    if( $text ne "" ) {
			print "  Text ->$text<-\n";
			if( $text =~ "container" ) {
			    print "   Adding container elements\n";
			    push @{$properties{ $property }->{elements}}, @container_elements;
			    $properties{ $property }->{addedContainers} = 1;
			}
			if( $text =~ "text" ) {
			    print "   Adding text content elements\n";
			    push @{$properties{ $property }->{elements}}, @text_content_elements;
			}
			if( $text =~ "graphics" ) {
			    print "   Adding graphics elements\n";
			    push @{$properties{ $property }->{elements}}, @graphics_elements;
			}
			if( $text =~ "primitives" ) {
			    print "   Adding filter primitive elements\n";
			    push @{$properties{ $property }->{elements}}, @filter_primitives;
			}
			if( $text =~ "shape" ) {
			    print "   Adding shape elements\n";
			    push @{$properties{ $property }->{elements}}, @shapes;
			}
			if( $text =~ "viewport" ) {
			    print "   Adding viewport elements\n";
			    push @{$properties{ $property }->{elements}}, @viewport;
			}
		    }
		} else {
		    print "  Not Elements: $t->[1]\n";
		}
	    }

	    if( $column == 5 and $t->[0] eq 'T') {
		# Fifth column is inherit value (simple text).
		my $inherit = $t->[1];
		$properties{ $property }->{inherit} = $inherit;
		print "  Inherit: $inherit\n";
	    }

	    if( $t->[0] eq 'E' and lc $t->[1] eq 'tr') {
		last;
	    }

	}
    }

}

# Adjustments
$properties{ "alignment-baseline" }->{default} = "auto";
$properties{ "color" }->{default} = "NO_DEFAULT";  # Depends on user agent.
$properties{ "font" }->{default} = "NO_DEFAULT";  # See individual font properties.
$properties{ "font-family" }->{default} = "NO_DEFAULT";
$properties{ "marker" }->{default} = "none"; # Same as marker-start, etc.
$properties{ "overflow" }->{default} = "hidden";  # On all but outermost <svg>, user agent style sheet sets this to hidden.

# This is the safest thing to do.... 
$properties{ "text-decoration" }->{default} = "NO_DEFAULT"; 
$properties{ "text-decoration" }->{inherit} = "no"; 

#
push @{$properties{ "color" }->{elements}}, @text_content_elements;
push @{$properties{ "color" }->{elements}}, @shapes;
push @{$properties{ "color" }->{elements}}, "stop", "feFlood", "feDiffuseLighting", "feSpecularLighting";

push @{$properties{ "color-interpolation-filters" }->{elements}}, "filter";

@{$properties{ "clip-rule" }->{elements}} = @graphics_elements;
push @{$properties{ "clip-rule" }->{elements}}, "clip-path";  # Can inherit

push @{$properties{ "display" }->{elements}}, @graphics_elements;

push @{$properties{ "image-rendering" }->{elements}}, "pattern", "image", "feImage";

# "title" isn't in the SVG spec (except for style sheets) but it is commonly supported by browsers
push @{$properties{ "title" }->{elements}}, @graphics_elements, "g";
$properties{ "title" }->{default} = "NO DEFAULT";
$properties{ "title" }->{inherit} = "no";

push @{$properties{ "visibility" }->{elements}}, @graphics_elements;


$properties{ "marker-end" }->{default} = $properties{ "marker-start" }->{default};
$properties{ "marker-mid" }->{default} = $properties{ "marker-start" }->{default};
$properties{ "marker-end" }->{inherit} = $properties{ "marker-start" }->{inherit};
$properties{ "marker-mid" }->{inherit} = $properties{ "marker-start" }->{inherit};
@{$properties{ "marker-end" }->{elements}} = @{$properties{ "marker-start" }->{elements}};
@{$properties{ "marker-mid" }->{elements}} = @{$properties{ "marker-start" }->{elements}};


# Inkscape uses CSS property 'line-height' even though this is not part of SVG spec.
push @{$properties{ "line-height" }->{elements}}, "text", "flowRoot", "flowPara";
$properties{ "line-height" }->{default} = "NO DEFAULT";
$properties{ "line-height" }->{inherit} = "no";

# Inkscape uses CSS property 'text-align' for flowed text. It is not an SVG 1.1 property
# but is found in SVG 1.2 Tiny.
push @{$properties{ "text-align" }->{elements}}, "flowRoot";
$properties{ "text-align" }->{default} = "start";
$properties{ "text-align" }->{inherit} = "yes";


# Output

open( DEFAULTS, ">css_defaults_new" ) or die "Couldn't open output";
open( ELEMENTS, ">cssprops_new" ) or die "Couldn't open output";

for $property ( sort keys %properties ) {
    print "$property:\t$properties{ $property }->{default}\t$properties{ $property }->{inherit}\n";
    # Must add container elements to all properties that are inherited.
    if( $properties{ $property }->{inherit} =~ "yes" ) {
	# But not if they have already been added
	if( !defined $properties{ $property }->{addedContainers} ) {
	    push @{$properties{ $property }->{elements}}, @container_elements;
	}
    }
    print "   @{ $properties{ $property }->{elements}}\n";

    print DEFAULTS "\"$property\" - \"$properties{ $property }->{default}\" - \"$properties{ $property }->{inherit}\"\n\n";
    print ELEMENTS "\"$property\" - ";
    my $first = 0;
    foreach (@{$properties{ $property }->{elements}}) {
	if( $first != 0 ) {
	    print ELEMENTS ",";
	}
	$first = 1;
	print ELEMENTS "\"$_\"";
    }
    print ELEMENTS "\n\n";
}

