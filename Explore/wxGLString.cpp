#include "wxGLString.h"
#include <iostream>

#ifdef __WXMAC__
#include "OpenGL/gl.h"
#else
#include <GL/gl.h>
#endif

#include "wx/wx.h"


GLuint* loadImage(wxImage* img)
{

	GLuint* ID=new GLuint[1];
	glGenTextures( 1, &ID[0] );

	glBindTexture( GL_TEXTURE_2D, *ID );


	glPixelStorei(GL_UNPACK_ALIGNMENT,   1   );

    const int w = img->GetWidth(), h = img->GetHeight();


    // note: must make a local copy before passing the data to OpenGL, as GetData() returns RGB
    // and we want the Alpha channel. Furthermore, the current rendering is black-on-white, we'll
    // convert it to an alpha channel by the way (not all platforms support transparency in wxDCs
    // so it's the easiest way to go)
    GLubyte *bitmapData=img->GetData();
    GLubyte *imageData;

    int bytesPerPixel = 4;

    int imageSize = w * h * bytesPerPixel;
    imageData=(GLubyte *)malloc(imageSize);

    int rev_val=h-1;

    for(int y=0; y<h; y++)
    {
        for(int x=0; x<w; x++)
        {
            imageData[(x+y*w)*bytesPerPixel+0] = 255;
            imageData[(x+y*w)*bytesPerPixel+1] = 255;
            imageData[(x+y*w)*bytesPerPixel+2] = 255;

            // alpha
            imageData[(x+y*w)*bytesPerPixel+3] = 255 - bitmapData[( x+(rev_val-y)*w)*3];
        }//next
    }//next

    glTexImage2D(GL_TEXTURE_2D, 0, bytesPerPixel, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, imageData);

    free(imageData);

	// set texture parameters as you wish
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return ID;

}


class TextTexture
{
    friend class wxGLString;
    friend class wxGLStringArray;
    friend class wxGLStringNumber;
private:
    GLuint* ID;
protected:

    GLuint* getID();

    TextTexture();
    TextTexture(wxBitmap& bmp);
    void load(wxImage* img);
public:


    ~TextTexture();

};

#if 0
#pragma mark -
#pragma mark TextGLDrawable implementation
#endif


TextGLDrawable::TextGLDrawable(TextTexture* image_arg)
{
    x=0;
    y=0;
    angle=0;

    xscale=1;
    yscale=1;

    xflip=false;
    yflip=false;

    if(image_arg!=NULL) setImage(image_arg);
    else image=NULL;

    tex_coord_x1 = 0;
    tex_coord_y1 = 1;
    tex_coord_x2 = 1;
    tex_coord_y2 = 0;
}

void TextGLDrawable::setFlip(bool x, bool y)
{
    xflip=x;
    yflip=y;
}

void TextGLDrawable::move(int x, int y)
{
    TextGLDrawable::x=x;
    TextGLDrawable::y=y;
}

void TextGLDrawable::scale(float x, float y)
{
    TextGLDrawable::xscale=x;
    TextGLDrawable::yscale=y;
}

void TextGLDrawable::scale(float k)
{
    TextGLDrawable::xscale=k;
    TextGLDrawable::yscale=k;
}

void TextGLDrawable::setImage(TextTexture* image)
{
    TextGLDrawable::image=image;
}

void TextGLDrawable::rotate(int angle)
{
    TextGLDrawable::angle=angle;
}

void TextGLDrawable::render()
{
    assert(image!=NULL);

    glPushMatrix();
    glTranslatef(x,y,0);
    if(xscale!=1 || yscale!=1) glScalef(xscale, yscale, 1);
    if(angle!=0) glRotatef(angle, 0,0,1);


    glBegin(GL_QUADS);

    glTexCoord2f(xflip? tex_coord_x2 : tex_coord_x1,
                 yflip? tex_coord_y2 : tex_coord_y1);
    glVertex2f( 0, 0 );

    glTexCoord2f(xflip? tex_coord_x1 : tex_coord_x2,
                 yflip? tex_coord_y2 : tex_coord_y1);
    glVertex2f( w, 0 );

    glTexCoord2f(xflip? tex_coord_x1 : tex_coord_x2,
                 yflip? tex_coord_y1 : tex_coord_y2);
    glVertex2f( w, h );

    glTexCoord2f(xflip? tex_coord_x2 : tex_coord_x1,
                 yflip? tex_coord_y1 : tex_coord_y2);
    glVertex2f( 0, h );

    glEnd();
    glPopMatrix();
}


#if 0
#pragma mark -
#pragma mark TextTexture implementation
#endif

TextTexture::TextTexture()
{
}

TextTexture::TextTexture(wxBitmap& bmp)
{
    wxImage img = bmp.ConvertToImage();
    load(&img);
}
void TextTexture::load(wxImage* img)
{
    ID=loadImage(img);
}

GLuint* TextTexture::getID()
{
    return ID;
}

TextTexture::~TextTexture()
{
    glDeleteTextures (1, ID);
}


#if 0
#pragma mark -
#pragma mark wxGLString implementation
#endif

wxGLString::wxGLString() : wxString(wxT("")), TextGLDrawable()
{
    img = NULL;
}
wxGLString::wxGLString(wxString message) : wxString(message), TextGLDrawable()
{
    img = NULL;
}
void wxGLString::operator=(wxString& string)
{
    (*((wxString*)this))=string;
}
void wxGLString::bind()
{
    glBindTexture(GL_TEXTURE_2D, img->getID()[0] );
}
void wxGLString::calculateSize(wxDC* dc)
{
    if(font.IsOk()) dc->SetFont(font);
    else dc->SetFont(wxSystemSettings::GetFont(wxSYS_SYSTEM_FONT));
    
    dc->GetTextExtent(*this, &w, &h);
}

void wxGLString::consolidate(wxDC* dc)
{
    calculateSize(dc);
    const int power_of_2_w = pow( 2, (int)ceil((float)log(w)/log(2.0)) );
    const int power_of_2_h = pow( 2, (int)ceil((float)log(h)/log(2.0)) );

    wxBitmap bmp(power_of_2_w, power_of_2_h);
    assert(bmp.IsOk());

    {
        wxMemoryDC temp_dc(bmp);

        temp_dc.SetBrush(*wxWHITE_BRUSH);
        temp_dc.Clear();

        if(font.IsOk()) temp_dc.SetFont(font);
        else temp_dc.SetFont(wxSystemSettings::GetFont(wxSYS_SYSTEM_FONT));
        
        temp_dc.DrawText(*this, 0, 0);
    }
    if(img != NULL) delete img;
    img = new TextTexture(bmp);

    TextGLDrawable::w = power_of_2_w;
    TextGLDrawable::h = power_of_2_h;
    TextGLDrawable::setImage(img);
}
void wxGLString::consolidateFromArray(wxDC* dc, int x, int y)
{
    dc->DrawText(*this, x, y);
}

void wxGLString::setFont(wxFont font)
{
    wxGLString::font = font;
}

void wxGLString::render(const int x, const int y)
{
    TextGLDrawable::move(x, y);
    TextGLDrawable::render();
}
wxGLString::~wxGLString()
{
    if(img != NULL) delete img;
}


#if 0
#pragma mark -
#pragma mark wxGLNumberRenderer implementation
#endif

wxGLNumberRenderer::wxGLNumberRenderer() : wxGLString( wxT("0 1 2 3 4 5 6 7 8 9 . - ") )
{
    number_location = new int[13];
}
wxGLNumberRenderer::~wxGLNumberRenderer()
{
    delete[] number_location;
}

void wxGLNumberRenderer::consolidate(wxDC* dc)
{
    wxGLString::consolidate(dc);

    if(font.IsOk()) dc->SetFont(font);
    else dc->SetFont(wxSystemSettings::GetFont(wxSYS_SYSTEM_FONT));
        
    number_location[0] = 0;
    number_location[1]  = dc->GetTextExtent(wxT("0 ")).GetWidth();
    number_location[2]  = dc->GetTextExtent(wxT("0 1 ")).GetWidth();
    number_location[3]  = dc->GetTextExtent(wxT("0 1 2 ")).GetWidth();
    number_location[4]  = dc->GetTextExtent(wxT("0 1 2 3 ")).GetWidth();
    number_location[5]  = dc->GetTextExtent(wxT("0 1 2 3 4 ")).GetWidth();
    number_location[6]  = dc->GetTextExtent(wxT("0 1 2 3 4 5 ")).GetWidth();
    number_location[7]  = dc->GetTextExtent(wxT("0 1 2 3 4 5 6 ")).GetWidth();
    number_location[8]  = dc->GetTextExtent(wxT("0 1 2 3 4 5 6 7 ")).GetWidth();
    number_location[9]  = dc->GetTextExtent(wxT("0 1 2 3 4 5 6 7 8 ")).GetWidth();
    number_location[10] = dc->GetTextExtent(wxT("0 1 2 3 4 5 6 7 8 9 ")).GetWidth();
    number_location[11] = dc->GetTextExtent(wxT("0 1 2 3 4 5 6 7 8 9 . ")).GetWidth();
    number_location[12] = dc->GetTextExtent(wxT("0 1 2 3 4 5 6 7 8 9 . - ")).GetWidth();

    space_w = dc->GetTextExtent(wxT(" ")).GetWidth();
}
void wxGLNumberRenderer::renderNumber(int i, int x, int y)
{
    wxString s;
    s << i;
    renderNumber(s, x, y);
}
void wxGLNumberRenderer::renderNumber(float f, int x, int y)
{
    wxString s;
    s << f;
    renderNumber(s, x, y);
}
void wxGLNumberRenderer::renderNumber(wxString s, int x, int y)
{

    const int full_string_w = TextGLDrawable::w;

    const int char_amount = s.Length();
    for(int c=0; c<char_amount; c++)
    {
        int charid = -1;

        char schar = s[c];
        switch(schar)
        {
            case '0' : charid = 0; break;
            case '1' : charid = 1; break;
            case '2' : charid = 2; break;
            case '3' : charid = 3; break;
            case '4' : charid = 4; break;
            case '5' : charid = 5; break;
            case '6' : charid = 6; break;
            case '7' : charid = 7; break;
            case '8' : charid = 8; break;
            case '9' : charid = 9; break;
            case '.' :
            case ',' : charid = 10; break;
            case '-' : charid = 11; break;
            default: printf("Warning: character %c unexpected in number!\n", schar); continue;
        }

        assert( charid != -1 );

        TextGLDrawable::tex_coord_x1 = (float)number_location[charid] / (float)full_string_w;
        TextGLDrawable::tex_coord_x2 = (float)(number_location[charid+1]-space_w) / (float)full_string_w;

        const int char_width = number_location[charid+1] - number_location[charid] - space_w;
        TextGLDrawable::w = char_width;


        TextGLDrawable::move(x, y);
        TextGLDrawable::render();

        x += char_width;
    } // next

    TextGLDrawable::w = full_string_w;
}

#if 0
#pragma mark -
#pragma mark wxGLStringArray implementation
#endif

wxGLStringArray::wxGLStringArray()
{
    img = NULL;
}
wxGLStringArray::wxGLStringArray(wxString strings_arg[], int amount)
{
    img = NULL;
    for(int n=0; n<amount; n++)
        strings.push_back( wxGLString(strings_arg[n]) );
}
wxGLStringArray::~wxGLStringArray()
{
    if(img != NULL) delete img;
}

wxGLString& wxGLStringArray::get(const int id)
{
    return strings[id];
}
void wxGLStringArray::bind()
{
    glBindTexture(GL_TEXTURE_2D, img->getID()[0] );
}
void wxGLStringArray::addString(wxString string)
{
    strings.push_back( wxGLString(string) );
}
void wxGLStringArray::setFont(wxFont font)
{
    wxGLStringArray::font = font;
}

void wxGLStringArray::consolidate(wxDC* dc)
{
    int x=0, y=0;

    // find how much space we need
    int longest_string = 0;

    const int amount = strings.size();
    for(int n=0; n<amount; n++)
    {
        strings[n].calculateSize(dc);
        y += strings[n].h;
        if(strings[n].w > longest_string) longest_string = strings[n].w;
    }//next

    const int power_of_2_w = pow( 2, (int)ceil((float)log(longest_string)/log(2.0)) );
    const int power_of_2_h = pow( 2, (int)ceil((float)log(y)/log(2.0)) );

    wxBitmap bmp(power_of_2_w, power_of_2_h);
    assert(bmp.IsOk());

    {
        wxMemoryDC temp_dc(bmp);

        temp_dc.SetBrush(*wxWHITE_BRUSH);
        temp_dc.Clear();

        y = 0;
        if(font.IsOk()) temp_dc.SetFont(font);
        else temp_dc.SetFont(wxSystemSettings::GetFont(wxSYS_SYSTEM_FONT));
        
        for(int n=0; n<amount; n++)
        {
            strings[n].consolidateFromArray(&temp_dc, 0, y);

            strings[n].tex_coord_x1 = (float)x/(float)power_of_2_w;
            strings[n].tex_coord_y1 = 1.0 - (float)y/(float)power_of_2_h;
            strings[n].tex_coord_x2 = (float)(x+strings[n].w)/(float)power_of_2_w;
            strings[n].tex_coord_y2 = 1.0 - (float)(y+strings[n].h)/(float)power_of_2_h;

            y += strings[n].h;
        }
    }
    if(img != NULL) delete img;
    img = new TextTexture(bmp);

    for(int n=0; n<amount; n++)
        strings[n].setImage(img);

}


