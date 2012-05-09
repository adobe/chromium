may2012-f2f-prototype
=====================

`may2012-f2f-prototype` branch contains various changes intended for presentation at the May 2012 CSS WG F2F meeting in Hamburg. This branch includes prototype and experimental work. It should not be considered final or ready for submission into the WebKit trunk.

The source for this prototype can be found at [chromium/may2012-f2f-prototype](https://github.com/adobe/chromium/tree/may2012-f2f-prototype)

Demos of prototype features in this build can be found at
* [web-platform/may2012-f2f-prototype](https://github.com/adobe/web-platform/tree/may2012-f2f-prototype/)
* [hg.csswg.org](https://hg.csswg.org/test/file/7e6cd72828e8/contributors/adobe/incoming/css3-regions) (location will change after tests have been reviewed and accepted)

General Changes
---------------
In order to make it easy to test the new experimental features, the prototype build enables CSS Regions and CSS Shaders by default. It only enables them the first time it runs, adding a new flag called "browser.adobe-may2012-prototype-features-enabled" in the Chromium's settings to prevent subsequent changes. Other developer builds of Chromium will also be affected by the changed settings for CSS Regions and CSS Shaders.

CSS Regions
-----------
The prototype includes support to specify *height: auto* and *width: auto* on Region elements. For more information please visit the [CSS Regions Specification](http://dev.w3.org/csswg/css3-regions/#regions-visual-formatting-steps).
Added experimental support for multi-column regions: for example, you can use "-webkit-columns: 2" to make a region layout the content inside it using two column.

CSS Exclusions
--------------
Experimental support for the CSS Exclusions processing model is included in the prototype. The model is explained in the [CSS Exclusions Specification](http://dev.w3.org/csswg/css3-exclusions/#exclusions-processing-model). Note, that only the "-webkit-wrap-flow: clear" property is implemented.

CSS Shaders
-----------
Most of the CSS Shaders code is already upstreamed to WebKit. We are looking forward to upstreaming all the CSS Shaders changes in the following weeks.

List of CSS Shaders changes in this branch:

+ [71443](https://bugs.webkit.org/show_bug.cgi?id=71443) - Added support for the "transform" parameters

+ [85113](https://bugs.webkit.org/show_bug.cgi?id=85113) - Make CSS Shaders render to texture framebuffers

+ [85086](https://bugs.webkit.org/show_bug.cgi?id=85086) - [CSS Shaders] Improve performance of CSS Shaders rendering - Partial implementation, sharing the shaders and the GraphicsContext3D between filtered elements.

+ Prototyped a way to rewrite the shaders, so that they don't get access to the element's texture. This is a recommended security measure to avoid timming attacks. For that we used the ANGLE project and all the changes are available in our [ANGLE branch](https://github.com/adobe/angle/tree/shader-rewriter).

CSS Compositing
-----------
+ Added support for css blending and alpha compositing with -webkit-blend-mode and -webkit-alpha-compositing for Chromium

+ Added support for additional blending modes in Canvas in WebKit and Safari

+ Added set of files that show proof-of-concept of these features in Safari, Chromium and a custom FireFox

CSS Transforms
-------------
The [CSS3 Transforms Module](http://dev.w3.org/csswg/css3-transforms/) provides the possibility to transform the two- and three-dimensional coordinate space of elements in the SVG and HTML namespace. This turns the SVG 'transform' attribute into a presentation attribute.

This change temporarily breaks SVG animations and SVG DOM access for the 'transform' attribute.

Windows Build
-------------
Note that if you are using the Windows build DirectX is needed, which can be downloaded from the following address: 
(http://www.microsoft.com/download/en/details.aspx?displaylang=en%3fWT.mc_id%3dMSCOM_DLC_US_DR_111SXX02959&id=35)

Downloads
---------
You can download both Mac and Windows builds of this branch [here](https://github.com/adobe/webkit/downloads).
