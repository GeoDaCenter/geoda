#ifndef _drawable_
#define _drawable_

#ifdef __WXMAC__
#include "OpenGL/gl.h"
#else
#include <GL/gl.h>
#endif

#include "wx/wx.h"
#include <vector>

class TextTexture;

/** base class for renderable elements. You won't create this one directly,
but may use its public members from wxGLString since it inherits from TextGLDrawable.
This class will be useful if you wish to apply effects to the text like rotation or
scaling. */
class TextGLDrawable
{
    friend class wxGLString;
    friend class wxGLStringArray;
    friend class wxGLStringNumber;
protected:

    int x,y, angle;
    float xscale, yscale;
    TextTexture* image;
    bool xflip, yflip;

    float tex_coord_x1, tex_coord_y1;
    float tex_coord_x2, tex_coord_y2;
    int w, h;

    TextGLDrawable(TextTexture* image=(TextTexture*)0);
    void render();
    void setImage(TextTexture* image);
    void move(int x, int y);
public:

    /** allows you to flip the rendering vertically and/or horizontally */
    void setFlip(bool x, bool y);

    /** scale the rendering , horizontally and vertically (allows stretching) */
    void scale(float x, float y);

    /** scale the rendering and keep the same aspect ratio */
    void scale(float k);

    /** reotate the rendering by 'angle' degrees */
    void rotate(int angle);

};

class wxGLStringArray;

/** wxGLString is the simplest class you can use. It draws a single string on a single line.
If you plan to render multiple strings, this class is not the fastest.

Use example :

wxGLString my_message(wxT("Hello World"));
...
if(first_render)
    my_message.consolidate(&dc);

glColor3f(0,0,0); // black text
my_message.bind();
my_message.render(x, y);
*/
class wxGLString : public wxString, public TextGLDrawable
{
protected:
    TextTexture* img;
    wxFont font;

    friend class wxGLStringArray;

    void calculateSize(wxDC* dc=NULL);
    void consolidateFromArray(wxDC* dc, int x, int y);
public:
    /** constructs an empty GLString. Set string later with operator=. */
    wxGLString();
    /** constructs a GLstring with 'message' as contents. */
    wxGLString(wxString message);
    ~wxGLString();

    /** call just before render() - binds the OpenGL. If you render the same string many
        times, or render from an array, bind only once, this will improve performance */
    void bind();

    /** set how to draw string for next consolidate() - has no immediate effect,
        you need to call consolidate() to get results  */
    void setFont(wxFont font);

    /** to be called after consolidate() only */
    void getStringExtent();

    /** consolidates the current string info into a GL string. call this after
     setting up strings, font and color (if necessary), and before rendering.
      The wxDC argument is only used to calculate text extents and will not be rendered on. */
    void consolidate(wxDC* dc);

    /** render this string at coordinates (x,y). Must be called after bind(). */
    void render(const int x, const int y);

    /** changes the string of this element - has no immediate effect,
     you need to call consolidate() to get results */
    void operator=(wxString& string);
};

/** This class allows rendering numbers.

Use example :

wxGLNumberRenderer glnumbers;
...
if(first_render)
    glnumbers.consolidate();

glColor3f(0,0,0); // black numbers
glnumbers.bind();
glnumbers.renderNumber( 3.141593f, x, y );
*/
class wxGLNumberRenderer : public wxGLString
{
    int* number_location;
    int space_w;
public:
    wxGLNumberRenderer();
    ~wxGLNumberRenderer();

    /** inits the class to be ready to render.
    The wxDC argument is only used to calculate text extents and will not be rendered on. */
    void consolidate(wxDC* dc);

    /** render this number at coordinates (x,y), where wxString s contains the string
     representation of a number. Must be called after bind(). */
    void renderNumber(wxString s, int x, int y);
    /** render this number at coordinates (x,y). Must be called after bind(). */
    void renderNumber(int i, int x, int y);
    /** render this number at coordinates (x,y). Must be called after bind(). */
    void renderNumber(float f, int x, int y);
};

/** This class is useful to render a serie of strings that are usually rendered at the same time.
It behaves exactly like wxGLString but is more efficient.


Use example :

wxGLStringArray my_messages();
my_messages.addString("wxMac");
my_messages.addString("wxGTK");
my_messages.addString("wxMSW");
...
if(first_render)
    my_messages.consolidate(&dc);

glColor3f(0,0,0); // black text
my_messages.bind();
my_messages.get(0).render( x, y      );
my_messages.get(1).render( x, y + 25 );
my_messages.get(2).render( x, y + 50 );
*/
class wxGLStringArray
{
    std::vector<wxGLString> strings;
    TextTexture* img;
    wxFont font;
public:
    /** constructs an empty array - add elements later using addString */
    wxGLStringArray();
    /** construct an array with 'strings_arg' elemnts in it */
    wxGLStringArray(wxString strings_arg[], int amount);
    ~wxGLStringArray();

    /** get a sub-element - useful mainly for rendering, e.g. my_array.get(0).render(x, y); */
    wxGLString& get(const int id);

    /** call just before render() - binds the OpenGL. If you render the same string many
     times, or render from an array, bind only once, this will improve performance */
    void bind();

    /** add a string to the list for next consolidate() - has no
     immediate effect, you need to call consolidate() to get results  */
    void addString(wxString string);

    /** set how to draw string for next consolidate() - has no immediate effect,
     you need to call consolidate() to get results  */
    void setFont(wxFont font);

    /** consolidates the current string info into a GL string. call this after
     setting up strings, font and color (if necessary), and before rendering.
     The wxDC argument is only used to calculate text extents and will not be rendered on.  */
    void consolidate(wxDC* dc);
};


#endif
