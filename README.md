[![Build Status](https://travis-ci.org/GeoDaCenter/geoda.svg?branch=master)](https://travis-ci.org/GeoDaCenter/geoda)

# Acknowledgements #

GeoDa TM is built upon several open source libraries and source-code files.

GeoDa is the flagship program of the GeoDa Center, following a long line of software tools developed by Dr. Luc Anselin. It is designed to implement techniques for exploratory spatial data analysis (ESDA) on lattice data (points and polygons). The free program provides a user friendly and graphical interface to methods of descriptive spatial data analysis, such as spatial autocorrelation statistics, as well as basic spatial regression functionality. The latest version contains several new features such as full space-time data support in all views, a new cartogram, a refined map movie, parallel coordinate plot, 3D visualization, conditional plots (and maps) and spatial regression.

Since its initial release in February 2003, GeoDa's user numbers have increased exponentially, as the chart and map of global users above shows. This includes lab users at universities such as Harvard, MIT, and Cornell. The user community and press embraced the program enthusiastically, calling it a "hugely important analytic tool," a "very fine piece of software," an "exciting development" and more.

# Build GeoDa #

Please read the detail instructions under directory BuildTools/

[Windows](BuildTools/windows/readme.md)

[Mac OSX](BuildTools/macosx/readme.md)

[Linux/Ubuntu](BuildTools/ubuntu/readme.md)

Note:  contributions of build scripts under other platforms are welcomed, please follow the structure of building script under BuildTools/.

# Internationalization #

GeoDa Internationalization (I18n) and Localization(L10n) project aims to provide an online tool that GeoDa users could help to translate the GeoDa UI to different languages.

For crowdsourcing, we use Google Spreadsheet with the public address [here](https://docs.google.com/spreadsheets/d/1iZa4wCIyTDlIRYoW7229YoZWKZ0lmIiOFsCJG3ZVw-s/edit?usp=sharing). Anyone can access this spreadsheet, and edit each translation.

Contributors:

* @corochasco

# Dependencies #

Below is a list of some of these that we'd like to acknowledge.

* GDAL Libraries, version 1.10

        License: X/MIT style Open Source license
        Authors: many
        Links: http://www.gdal.org/
    
* Boost Libraries, version 1.53

        License: Boost Software License - Version 1.0
        Authors: many
        Links: http://www.boost.org/
              http://www.boost.org/LICENSE_1_0.txt

* Boost.Polygon Voronoi Library, Boost version 1.53

        License: Boost Software License - Version 1.0
        Author: Andrii Sydorchuk
        Links: http://www.boost.org/
              http://www.boost.org/LICENSE_1_0.txt

* wxWidgets Cross-Platform GUI Library, version 2.9.4

        License: The wxWindows Library Licence
        Authors: Julian Smart, Robert Roebling, and others
        Links: http://www.wxwidgets.org/
                http://www.opensource.org/licenses/wxwindows.php

* CLAPACK Linear Algebra Libraries, version 3.2.1

        Authors: many
        License: Custom by University of Tennessee
        Links: http://www.netlib.org/clapack/
                http://www.netlib.org/lapack/lapack-3.2/LICENSE

* Approximate Nearest Neighbor Library, version 0.1

        Note: Full source of 0.1 release included in kNN directory
        Authors: Sunil Arya and David Mount
        License: See kNN/AHH.h in included source files
        Links: http://www.cs.umd.edu/~mount/ANN/

* FastArea.c++ source code

        Note: We have based the source for functions findArea and
        ComputeArea2D in our file GenGeomAlgs.h from FastArea.c++
        in Journal of Graphics Tools, 7(2):9-13, 2002
        Author: Daniel Sunday
        License: unknown
        Links: http://www.tandfonline.com/doi/abs/10.1080/10867651.2002.10487556
