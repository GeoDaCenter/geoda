#include <cmath>
#include <algorithm>


#include "GenColor.h"

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884L
#endif
#ifndef SQR
#define SQR(x) ((x)*(x))
#endif
#ifndef POW2
#define POW2(x) SQR(x)
#endif
#ifndef POW3
#define POW3(x) ((x)*(x)*(x))
#endif
#ifndef POW4
#define POW4(x) (POW2(x)*POW2(x))
#endif
#ifndef POW7
#define POW7(x) (POW3(x)*POW3(x)*(x))
#endif
#ifndef DegToRad
#define DegToRad(x) ((x)*M_PI/180)
#endif
#ifndef RadToDeg
#define RadToDeg(x) ((x)/M_PI*180)
#endif

#ifndef cbrt
#define cbrt(x) pow(x, 1.0/3.0)
#endif

namespace ColorSpace {
    Rgb::Rgb() {}
    Rgb::Rgb(double r, double g, double b) : r(r), g(g), b(b) {}
    void Rgb::Initialize(Rgb *color) {
        RgbConverter::ToColorSpace(color, this);
    }
    void Rgb::ToRgb(Rgb *color) {
        RgbConverter::ToColor(color, this);
    }
    void Rgb::Copy(IColorSpace *color) {
        Rgb *rgb = static_cast<Rgb*>(color);
        rgb->r = r;
        rgb->g = g;
        rgb->b = b;
    }


    Xyz::Xyz() {}
    Xyz::Xyz(double x, double y, double z) : x(x), y(y), z(z) {}
    void Xyz::Initialize(Rgb *color) {
        XyzConverter::ToColorSpace(color, this);
    }
    void Xyz::ToRgb(Rgb *color) {
        XyzConverter::ToColor(color, this);
    }
    void Xyz::Copy(IColorSpace *color) {
        Xyz *xyz = static_cast<Xyz*>(color);
        xyz->x = x;
        xyz->y = y;
        xyz->z = z;
    }

    Hsl::Hsl() {}
    Hsl::Hsl(double h, double s, double l) : h(h), s(s), l(l) {}
    void Hsl::Initialize(Rgb *color) {
        HslConverter::ToColorSpace(color, this);
    }
    void Hsl::ToRgb(Rgb *color) {
        HslConverter::ToColor(color, this);
    }
    void Hsl::Copy(IColorSpace *color) {
        Hsl *hsl = static_cast<Hsl*>(color);
        hsl->h = h;
        hsl->s = s;
        hsl->l = l;
    }

    Lab::Lab() {}
    Lab::Lab(double l, double a, double b) : l(l), a(a), b(b) {}
    void Lab::Initialize(Rgb *color) {
        LabConverter::ToColorSpace(color, this);
    }
    void Lab::ToRgb(Rgb *color) {
        LabConverter::ToColor(color, this);
    }
    void Lab::Copy(IColorSpace *color) {
        Lab *lab = static_cast<Lab*>(color);
        lab->l = l;
        lab->a = a;
        lab->b = b;
    }

    Lch::Lch() {}
    Lch::Lch(double l, double c, double h) : l(l), c(c), h(h) {}
    void Lch::Initialize(Rgb *color) {
        LchConverter::ToColorSpace(color, this);
    }
    void Lch::ToRgb(Rgb *color) {
        LchConverter::ToColor(color, this);
    }
    void Lch::Copy(IColorSpace *color) {
        Lch *lch = static_cast<Lch*>(color);
        lch->l = l;
        lch->c = c;
        lch->h = h;
    }

    Luv::Luv() {}
    Luv::Luv(double l, double u, double v) : l(l), u(u), v(v) {}
    void Luv::Initialize(Rgb *color) {
        LuvConverter::ToColorSpace(color, this);
    }
    void Luv::ToRgb(Rgb *color) {
        LuvConverter::ToColor(color, this);
    }
    void Luv::Copy(IColorSpace *color) {
        Luv *luv = static_cast<Luv*>(color);
        luv->l = l;
        luv->u = u;
        luv->v = v;
    }

    Yxy::Yxy() {}
    Yxy::Yxy(double y1, double x, double y2) : y1(y1), x(x), y2(y2) {}
    void Yxy::Initialize(Rgb *color) {
        YxyConverter::ToColorSpace(color, this);
    }
    void Yxy::ToRgb(Rgb *color) {
        YxyConverter::ToColor(color, this);
    }
    void Yxy::Copy(IColorSpace *color) {
        Yxy *yxy = static_cast<Yxy*>(color);
        yxy->y1 = y1;
        yxy->x = x;
        yxy->y2 = y2;
    }

    Cmy::Cmy() {}
    Cmy::Cmy(double c, double m, double y) : c(c), m(m), y(y) {}
    void Cmy::Initialize(Rgb *color) {
        CmyConverter::ToColorSpace(color, this);
    }
    void Cmy::ToRgb(Rgb *color) {
        CmyConverter::ToColor(color, this);
    }
    void Cmy::Copy(IColorSpace *color) {
        Cmy *cmy = static_cast<Cmy*>(color);
        cmy->c = c;
        cmy->m = m;
        cmy->y = y;
    }

    Cmyk::Cmyk() {}
    Cmyk::Cmyk(double c, double m, double y, double k) : c(c), m(m), y(y), k(k) {}
    void Cmyk::Initialize(Rgb *color) {
        CmykConverter::ToColorSpace(color, this);
    }
    void Cmyk::ToRgb(Rgb *color) {
        CmykConverter::ToColor(color, this);
    }
    void Cmyk::Copy(IColorSpace *color) {
        Cmyk *cmyk = static_cast<Cmyk*>(color);
        cmyk->c = c;
        cmyk->m = m;
        cmyk->y = y;
        cmyk->k = k;
    }

    Hsv::Hsv() {}
    Hsv::Hsv(double h, double s, double v) : h(h), s(s), v(v) {}
    void Hsv::Initialize(Rgb *color) {
        HsvConverter::ToColorSpace(color, this);
    }
    void Hsv::ToRgb(Rgb *color) {
        HsvConverter::ToColor(color, this);
    }
    void Hsv::Copy(IColorSpace *color) {
        Hsv *hsv = static_cast<Hsv*>(color);
        hsv->h = h;
        hsv->s = s;
        hsv->v = v;
    }

    Hsb::Hsb() {}
    Hsb::Hsb(double h, double s, double b) : h(h), s(s), b(b) {}
    void Hsb::Initialize(Rgb *color) {
        HsbConverter::ToColorSpace(color, this);
    }
    void Hsb::ToRgb(Rgb *color) {
        HsbConverter::ToColor(color, this);
    }
    void Hsb::Copy(IColorSpace *color) {
        Hsb *hsb = static_cast<Hsb*>(color);
        hsb->h = h;
        hsb->s = s;
        hsb->b = b;
    }

    HunterLab::HunterLab() {}
    HunterLab::HunterLab(double l, double a, double b) : l(l), a(a), b(b) {}
    void HunterLab::Initialize(Rgb *color) {
        HunterLabConverter::ToColorSpace(color, this);
    }
    void HunterLab::ToRgb(Rgb *color) {
        HunterLabConverter::ToColor(color, this);
    }
    void HunterLab::Copy(IColorSpace *color) {
        HunterLab *lab = static_cast<HunterLab*>(color);
        lab->l = l;
        lab->a = a;
        lab->b = b;
    }

    double Hue_2_RGB(double v1, double v2, double vh) {
        if (vh < 0) vh += 1;
        if (vh > 1) vh -= 1;
        if (6 * vh < 1) return v1 + (v2 - v1) * 6 * vh;
        if (2 * vh < 1) return v2;
        if (3 * vh < 2) return v1 + (v2 - v1)*(2.0 / 3.0 - vh) * 6;
        return v1;
    }


    void RgbConverter::ToColorSpace(Rgb *color, Rgb *item) {
        item->r = color->r;
        item->g = color->g;
        item->b = color->b;
    }
    void RgbConverter::ToColor(Rgb *color, Rgb *item) {
        color->r = item->r;
        color->g = item->g;
        color->b = item->b;
    }

    void XyzConverter::ToColorSpace(Rgb *color, Xyz *item) {
        double r = color->r / 255.0;
        double g = color->g / 255.0;
        double b = color->b / 255.0;

        r = ((r > 0.04045) ? pow((r + 0.055) / 1.055, 2.4) : (r / 12.92)) * 100.0;
        g = ((g > 0.04045) ? pow((g + 0.055) / 1.055, 2.4) : (g / 12.92)) * 100.0;
        b = ((b > 0.04045) ? pow((b + 0.055) / 1.055, 2.4) : (b / 12.92)) * 100.0;

        item->x = r*0.4124564 + g*0.3575761 + b*0.1804375;
        item->y = r*0.2126729 + g*0.7151522 + b*0.0721750;
        item->z = r*0.0193339 + g*0.1191920 + b*0.9503041;
    }
    void XyzConverter::ToColor(Rgb *color, Xyz *item) {
        double x = item->x / 100.0;
        double y = item->y / 100.0;
        double z = item->z / 100.0;

        double r = x * 3.2404542 + y * -1.5371385 + z * -0.4985314;
        double g = x * -0.9692660 + y * 1.8760108 + z * 0.0415560;
        double b = x * 0.0556434 + y * -0.2040259 + z * 1.0572252;

        r = ((r > 0.0031308) ? (1.055*pow(r, 1 / 2.4) - 0.055) : (12.92*r)) * 255.0;
        g = ((g > 0.0031308) ? (1.055*pow(g, 1 / 2.4) - 0.055) : (12.92*g)) * 255.0;
        b = ((b > 0.0031308) ? (1.055*pow(b, 1 / 2.4) - 0.055) : (12.92*b)) * 255.0;
        
        color->r = r;
        color->g = g;
        color->b = b;
    }
    const double XyzConverter::eps = 216.0 / 24389.0;
    const double XyzConverter::kappa = 24389.0 / 27.0;
    const Xyz XyzConverter::whiteReference(95.047, 100.000, 108.883);

    void HslConverter::ToColorSpace(Rgb *color, Hsl *item) {
        double r = color->r / 255.0;
        double g = color->g / 255.0;
        double b = color->b / 255.0;

        double min = std::min(r, std::min(g, b));
        double max = std::max(r, std::max(g, b));
        double delta = max - min;
        
        item->l = (max + min) / 2;
        if (delta == 0)
        {
            item->h = item->s = 0;
        }
        else {
            if (item->l < 0.5) {
                item->s = delta / (max + min) * 100;
            }
            else {
                item->s = delta / (1 - std::abs(2 * item->l - 1)) * 100;
            }

            if (r == max) {
                item->h =  (g - b) / delta;
            }
            else if (g == max) {
                item->h = (b - r) / delta + 2;
            }
            else if (b == max) {
                item->h = (r - g) / delta + 4;
            }
            item->h = fmod(60 * item->h + 360, 360);
        }
        item->l *= 100;
    }
    void HslConverter::ToColor(Rgb *color, Hsl *item) {
        double h = item->h / 360;
        double s = item->s / 100;
        double l = item->l / 100;

        if (item->s == 0) {
            color->r = color->g = color->b = item->l * 255;
        }
        else {
            double temp1, temp2;

            temp2 = (l < 0.5) ? (l*(1 + s)) : (l + s - (s*l));
            temp1 = 2 * l - temp2;

            color->r = 255 * Hue_2_RGB(temp1, temp2, h + 1.0 / 3.0);
            color->g = 255 * Hue_2_RGB(temp1, temp2, h);
            color->b = 255 * Hue_2_RGB(temp1, temp2, h - 1.0 / 3.0);
        }
    }

    void LabConverter::ToColorSpace(Rgb *color, Lab *item) {
        Xyz xyz;

        XyzConverter::ToColorSpace(color, &xyz);

        double x = xyz.x / 95.047;
        double y = xyz.y / 100.00;
        double z = xyz.z / 108.883;

        x = (x > 0.008856) ? cbrt(x) : (7.787 * x + 16.0 / 116.0);
        y = (y > 0.008856) ? cbrt(y) : (7.787 * y + 16.0 / 116.0);
        z = (z > 0.008856) ? cbrt(z) : (7.787 * z + 16.0 / 116.0);

        item->l = (116.0 * y) - 16;
        item->a = 500 * (x - y);
        item->b = 200 * (y - z);
    }
    void LabConverter::ToColor(Rgb *color, Lab *item) {
        double y = (item->l + 16.0) / 116.0;
        double x = item->a / 500.0 + y;
        double z = y - item->b / 200.0;

        double x3 = POW3(x);
        double y3 = POW3(y);
        double z3 = POW3(z);

        x = ((x3 > 0.008856) ? x3 : ((x - 16.0 / 116.0) / 7.787)) * 95.047;
        y = ((y3 > 0.008856) ? y3 : ((y - 16.0 / 116.0) / 7.787)) * 100.0;
        z = ((z3 > 0.008856) ? z3 : ((z - 16.0 / 116.0) / 7.787)) * 108.883;

        Xyz xyz(x, y, z);
        XyzConverter::ToColor(color, &xyz);
    }

    void LchConverter::ToColorSpace(Rgb *color, Lch *item) {
        Lab lab;

        LabConverter::ToColorSpace(color, &lab);
        double l = lab.l;
        double c = sqrt(lab.a*lab.a + lab.b*lab.b);
        double h = atan2(lab.b, lab.a);

        h = h / M_PI * 180;
        if (h < 0) {
            h += 360;
        }
        else if (h >= 360) {
            h -= 360;
        }

        item->l = l;
        item->c = c;
        item->h = h;
    }
    void LchConverter::ToColor(Rgb *color, Lch *item) {
        Lab lab;

        item->h = item->h * M_PI / 180;

        lab.l = item->l;
        lab.a = cos(item->h)*item->c;
        lab.b = sin(item->h)*item->c;

        LabConverter::ToColor(color, &lab);
    }

    void LuvConverter::ToColorSpace(Rgb *color, Luv *item) {
        const Xyz &white = XyzConverter::whiteReference;
        Xyz xyz;

        XyzConverter::ToColorSpace(color, &xyz);
        double y = xyz.y / white.y;
        double temp = (xyz.x + 15 * xyz.y + 3 * xyz.z);
        double tempr = (white.x + 15 * white.y + 3 * white.z);

        item->l = (y > XyzConverter::eps) ? (116 * cbrt(y) - 16) : (XyzConverter::kappa*y);
        item->u = 52 * item->l * (((temp > 1e-3) ? (xyz.x / temp) : 0) - white.x / tempr);
        item->v = 117 * item->l * (((temp > 1e-3) ? (xyz.y / temp) : 0) - white.y / tempr);
    }
    void LuvConverter::ToColor(Rgb *color, Luv *item) {
        const Xyz &white = XyzConverter::whiteReference;
        Xyz xyz;

        double y = (item->l > XyzConverter::eps*XyzConverter::kappa) ? POW3((item->l + 16) / 116) : (item->l / XyzConverter::kappa);
        double tempr = white.x + 15 * white.y + 3 * white.z;
        double up = 4 * white.x / tempr;
        double vp = 9 * white.y / tempr;

        double a = 1. / 3. * (52 * item->l / (item->u + 13 * item->l*up) - 1);
        double b = -5 * y;
        double x = (y*(39 * item->l / (item->v + 13 * item->l*vp) - 5) - b) / (a + 1. / 3.);
        double z = x*a + b;

        xyz.x = x * 100;
        xyz.y = y * 100;
        xyz.z = z * 100;

        XyzConverter::ToColor(color, &xyz);
    }

    void YxyConverter::ToColorSpace(Rgb *color, Yxy *item) {
        Xyz xyz;

        XyzConverter::ToColorSpace(color, &xyz);
        double temp = xyz.x + xyz.y + xyz.z;
        item->y1 = xyz.y;
        item->x = (temp==0) ? 0 : (xyz.x / temp);
        item->y2 = (temp==0) ? 0 : (xyz.y / temp);
    }
    void YxyConverter::ToColor(Rgb *color, Yxy *item) {
        Xyz xyz;

        xyz.x = item->x*(item->y1 / item->y2);
        xyz.y = item->y1;
        xyz.z = (1 - item->x - item->y2)*(item->y1 / item->y2);
        XyzConverter::ToColor(color, &xyz);
    }

    void CmyConverter::ToColorSpace(Rgb *color, Cmy *item) {
        item->c = 1 - color->r / 255;
        item->m = 1 - color->g / 255;
        item->y = 1 - color->b / 255;
    }
    void CmyConverter::ToColor(Rgb *color, Cmy *item) {
        color->r = (1 - item->c) * 255;
        color->g = (1 - item->m) * 255;
        color->b = (1 - item->y) * 255;
    }

    void CmykConverter::ToColorSpace(Rgb *color, Cmyk *item) {
        Cmy cmy;

        CmyConverter::ToColorSpace(color, &cmy);
        double k = 1.0;
        k = std::min(k, cmy.c);
        k = std::min(k, cmy.m);
        k = std::min(k, cmy.y);

        item->k = k;
        if (std::abs(item->k - 1) < 1e-3) {
            item->c = 0;
            item->m = 0;
            item->y = 0;
        }
        else {
            item->c = (cmy.c - k) / (1 - k);
            item->m = (cmy.m - k) / (1 - k);
            item->y = (cmy.y - k) / (1 - k);
        }
    }
    void CmykConverter::ToColor(Rgb *color, Cmyk *item) {
        Cmy cmy;

        cmy.c = item->c * (1 - item->k) + item->k;
        cmy.m = item->m * (1 - item->k) + item->k;
        cmy.y = item->y * (1 - item->k) + item->k;
        CmyConverter::ToColor(color, &cmy);
    }

    void HsvConverter::ToColorSpace(Rgb *color, Hsv *item) {
        double r = color->r / 255.0;
        double g = color->g / 255.0;
        double b = color->b / 255.0;

        double min = std::min(r, std::min(g, b));
        double max = std::max(r, std::max(g, b));
        double delta = max - min;

        item->v = max;
        item->s = (max > 1e-3) ? (delta / max) : 0;

        if (delta == 0) {
            item->h = 0;
        }
        else {
            if (r == max) {
                item->h = (g - b) / delta;
            }
            else if (g == max) {
                item->h = 2 + (b - r) / delta;
            }
            else if (b == max) {
                item->h = 4 + (r - g) / delta;
            }

            item->h *= 60;
            item->h = fmod(item->h + 360, 360);
        }
    }
    void HsvConverter::ToColor(Rgb *color, Hsv *item) {
        int range = (int)floor(item->h / 60);
        double c = item->v*item->s;
        double x = c*(1 - std::abs(fmod(item->h / 60, 2) - 1));
        double m = item->v - c;

        switch (range) {
            case 0:
                color->r = (c + m) * 255;
                color->g = (x + m) * 255;
                color->b = m * 255;
                break;
            case 1:
                color->r = (x + m) * 255;
                color->g = (c + m) * 255;
                color->b = m * 255;
                break;
            case 2:
                color->r = m * 255;
                color->g = (c + m) * 255;
                color->b = (x + m) * 255;
                break;
            case 3:
                color->r = m * 255;
                color->g = (x + m) * 255;
                color->b = (c + m) * 255;
                break;
            case 4:
                color->r = (x + m) * 255;
                color->g = m * 255;
                color->b = (c + m) * 255;
                break;
            default:        // case 5:
                color->r = (c + m) * 255;
                color->g = m * 255;
                color->b = (x + m) * 255;
                break;
        }
    }

    void HsbConverter::ToColorSpace(Rgb *color, Hsb *item) {
        Hsv hsv;

        HsvConverter::ToColorSpace(color, &hsv);
        item->h = hsv.h;
        item->s = hsv.s;
        item->b = hsv.v;
    }
    void HsbConverter::ToColor(Rgb *color, Hsb *item) {
        Hsv hsv;

        hsv.h = item->h;
        hsv.s = item->s;
        hsv.v = item->b;
        HsvConverter::ToColor(color, &hsv);
    }

    void HunterLabConverter::ToColorSpace(Rgb *color, HunterLab *item) {
        Xyz xyz;

        XyzConverter::ToColorSpace(color, &xyz);
        item->l = 10.0*sqrt(xyz.y);
        item->a = (xyz.y != 0) ? (17.5*(1.02*xyz.x - xyz.y) / sqrt(xyz.y)) : 0;
        item->b = (xyz.y != 0) ? (7.0*(xyz.y - 0.847*xyz.z) / sqrt(xyz.y)) : 0;
    }
    void HunterLabConverter::ToColor(Rgb *color, HunterLab *item) {
        double x = (item->a / 17.5) *(item->l / 10.0);
        double y = item->l*item->l / 100;
        double z = item->b / 7.0 * item->l / 10.0;

        Xyz xyz((x+y)/1.02, y, -(z-y)/0.847);
        XyzConverter::ToColor(color, &xyz);
    }

    Rgb* LerpColorLCH(Rgb &a, Rgb &b, float t)
    {
        Lch lch_a, lch_b;
        a.To<Lch>(&lch_a);
        b.To<Lch>(&lch_b);
        
        // L: ranges 0 no lightness 100 maximu lightness
        // C: 0 center of the circle/unsaturated, 100 edge of the circle/saturated
        // H: 0 degree red, 90 degree yellow, 180 green 270 blue 360 degree back to 0
        
        Lch lch_c;
        lch_c.l = lch_a.l + (lch_b.l - lch_a.l) * t;
        lch_c.c = lch_a.c + (lch_b.c - lch_b.c) * t;
        lch_c.h = lch_a.h + (lch_b.h - lch_b.h) * t;
        
        Rgb* c = new Rgb;
        lch_c.To<Rgb>(c);
        
        return c;
    }

    std::vector<Rgb> ColorSpectrumLCH(Rgb &a, Rgb &b, size_t n)
    {
        std::vector<Rgb> result(n);
        
        Lch lch_a, lch_b;
        a.To<Lch>(&lch_a);
        b.To<Lch>(&lch_b);
        
        // L: ranges 0 no lightness 100 maximu lightness
        // C: 0 center of the circle/unsaturated, 100 edge of the circle/saturated
        // H: 0 degree red, 90 degree yellow, 180 green 270 blue 360 degree back to 0
        
        double l_itv = (lch_b.l - lch_a.l) / (n-1);
        double c_itv = (lch_b.c - lch_a.c) / (n-1);
        double h_itv = (lch_b.h - lch_a.h) / (n-1);
        
        for (size_t i=0; i<n; ++i) {
            Lch lch_c;
            lch_c.l = lch_a.l + l_itv * i;
            lch_c.c = lch_a.c + c_itv * i;
            lch_c.h = lch_a.h + h_itv * i;
            
            lch_c.To<Rgb>(&result[i]);
        }
        
        return result;
    }

    std::vector<Rgb> ColorSpectrumLUV(Rgb &a, Rgb &b, size_t n)
    {
        std::vector<Rgb> result(n);
        
        Luv luv_a, luv_b;
        a.To<Luv>(&luv_a);
        b.To<Luv>(&luv_b);
        
        // L: ranges 0 no lightness 100 maximu lightness
        // C: 0 center of the circle/unsaturated, 100 edge of the circle/saturated
        // H: 0 degree red, 90 degree yellow, 180 green 270 blue 360 degree back to 0
        
        double l_itv = (luv_b.l - luv_a.l) / (n-1);
        double u_itv = (luv_b.u - luv_a.u) / (n-1);
        double v_itv = (luv_b.v - luv_a.v) / (n-1);
        
        for (size_t i=0; i<n; ++i) {
            Luv luv_c;
            luv_c.l = luv_a.l + l_itv * i;
            luv_c.u = luv_a.u + u_itv * i;
            luv_c.v = luv_a.v + v_itv * i;
            
            luv_c.To<Rgb>(&result[i]);
        }
        
        return result;
    }

    std::vector<Rgb> ColorSpectrumRgb(Rgb &a, Rgb &b, size_t n)
    {
        std::vector<Rgb> result(n);
         
        double r_itv = (b.r - a.r) / (n-1);
        double g_itv = (b.g - a.g) / (n-1);
        double b_itv = (b.b - a.b) / (n-1);
        
        for (size_t i=0; i<n; ++i) {
            result[i].r = a.r + r_itv * i;
            result[i].g = a.g + g_itv * i;
            result[i].b = a.b + b_itv * i;
        }
        
        return result;
    }

    std::vector<Rgb> ColorSpectrumHSV(Rgb &a, Rgb &b, size_t n)
    {
        std::vector<Rgb> result(n);
         
        Hsv hsv_a, hsv_b;
        a.To<Hsv>(&hsv_a);
        b.To<Hsv>(&hsv_b);
        
        double h_itv = (hsv_b.h - hsv_a.h) / (n-1);
        double s_itv = (hsv_b.s - hsv_a.s) / (n-1);
        double v_itv = (hsv_b.v - hsv_a.v) / (n-1);
        
        for (size_t i=0; i<n; ++i) {
            Hsv hsv_c;
            hsv_c.h = hsv_a.h + h_itv * i;
            hsv_c.s = hsv_a.s + s_itv * i;
            hsv_c.v = hsv_a.v + v_itv * i;
            
            hsv_c.To<Rgb>(&result[i]);
        }
        
        return result;
    }
}


