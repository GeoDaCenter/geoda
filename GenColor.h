#ifndef __GEODA_CENTER_GENCOLOR_H__
#define __GEODA_CENTER_GENCOLOR_H__

#include <vector>
#include <typeinfo>

namespace ColorSpace {
    struct Rgb;
    struct Xyz;
    struct Hsl;
    struct Lab;
    struct Lch;
    struct Luv;
    struct Yxy;
    struct Cmy;
    struct Cmyk;
    struct Hsv;
    struct Hsb;
    struct HunterLab;

    // conversion
    template <typename TColorSpace>
    struct IConverter {
        static void ToColorSpace(Rgb *color, TColorSpace *item);
        static void ToColor(Rgb *color, TColorSpace *item);
    };

    template <>
    struct IConverter<Rgb> {
        static void ToColorSpace(Rgb *color, Rgb *item);
        static void ToColor(Rgb *color, Rgb *item);
    };
    typedef IConverter<Rgb> RgbConverter;

    template <>
    struct IConverter<Xyz> {
        static void ToColorSpace(Rgb *color, Xyz *item);
        static void ToColor(Rgb *color, Xyz *item);
        static const double eps;
        static const double kappa;
        static const Xyz whiteReference;
    };
    typedef IConverter<Xyz> XyzConverter;

    template <>
    struct IConverter<Hsl> {
        static void ToColorSpace(Rgb *color, Hsl *item);
        static void ToColor(Rgb *color, Hsl *item);
    };
    typedef IConverter<Hsl> HslConverter;

    template <>
    struct IConverter<Lab> {
        static void ToColorSpace(Rgb *color, Lab *item);
        static void ToColor(Rgb *color, Lab *item);
    };
    typedef IConverter<Lab> LabConverter;

    template <>
    struct IConverter<Lch> {
        static void ToColorSpace(Rgb *color, Lch *item);
        static void ToColor(Rgb *color, Lch *item);
    };
    typedef IConverter<Lch> LchConverter;

    template <>
    struct IConverter<Luv> {
        static void ToColorSpace(Rgb *color, Luv *item);
        static void ToColor(Rgb *color, Luv *item);
    };
    typedef IConverter<Luv> LuvConverter;

    template <>
    struct IConverter<Yxy> {
        static void ToColorSpace(Rgb *color, Yxy *item);
        static void ToColor(Rgb *color, Yxy *item);
    };
    typedef IConverter<Yxy> YxyConverter;

    template <>
    struct IConverter<Cmy> {
        static void ToColorSpace(Rgb *color, Cmy *item);
        static void ToColor(Rgb *color, Cmy *item);
    };
    typedef IConverter<Cmy> CmyConverter;

    template <>
    struct IConverter<Cmyk> {
        static void ToColorSpace(Rgb *color, Cmyk *item);
        static void ToColor(Rgb *color, Cmyk *item);
    };
    typedef IConverter<Cmyk> CmykConverter;

    template <>
    struct IConverter<Hsv> {
        static void ToColorSpace(Rgb *color, Hsv *item);
        static void ToColor(Rgb *color, Hsv *item);
    };
    typedef IConverter<Hsv> HsvConverter;

    template <>
    struct IConverter<Hsb> {
        static void ToColorSpace(Rgb *color, Hsb *item);
        static void ToColor(Rgb *color, Hsb *item);
    };
    typedef IConverter<Hsb> HsbConverter;

    template <>
    struct IConverter<HunterLab> {
        static void ToColorSpace(Rgb *color, HunterLab *item);
        static void ToColor(Rgb *color, HunterLab *item);
    };
    typedef IConverter<HunterLab> HunterLabConverter;

    // conversion api
    struct IColorSpace {
        IColorSpace() {}
        virtual ~IColorSpace() {}

        virtual void Initialize(Rgb *color) = 0;
        virtual void ToRgb(Rgb *color) = 0;
        virtual void Copy(IColorSpace *color) = 0;

        template <typename TColorSpace>
        void To(TColorSpace *color);
    };


    struct Rgb : public IColorSpace {
        double r, g, b;

        Rgb();
        Rgb(double r, double g, double b);

        virtual void Initialize(Rgb *color);
        virtual void ToRgb(Rgb *color);
        virtual void Copy(IColorSpace *color);
    };

    struct Xyz : public IColorSpace {
        double x, y, z;

        Xyz();
        Xyz(double x, double y, double z);

        virtual void Initialize(Rgb *color);
        virtual void ToRgb(Rgb *color);
        virtual void Copy(IColorSpace *color);
    };

    struct Hsl : public IColorSpace {
        double h, s, l;

        Hsl();
        Hsl(double h, double s, double l);

        virtual void Initialize(Rgb *color);
        virtual void ToRgb(Rgb *color);
        virtual void Copy(IColorSpace *color);
    };

    struct Lab : public IColorSpace {
        double l, a, b;

        Lab();
        Lab(double l, double a, double b);

        virtual void Initialize(Rgb *color);
        virtual void ToRgb(Rgb *color);
        virtual void Copy(IColorSpace *color);
    };

    struct Lch : public IColorSpace {
        double l, c, h;

        Lch();
        Lch(double l, double c, double h);

        virtual void Initialize(Rgb *color);
        virtual void ToRgb(Rgb *color);
        virtual void Copy(IColorSpace *color);
    };

    struct Luv : public IColorSpace {
        double l, u, v;

        Luv();
        Luv(double l, double u, double v);

        virtual void Initialize(Rgb *color);
        virtual void ToRgb(Rgb *color);
        virtual void Copy(IColorSpace *color);
    };

    struct Yxy : public IColorSpace {
        double y1, x, y2;

        Yxy();
        Yxy(double y1, double x, double y2);

        virtual void Initialize(Rgb *color);
        virtual void ToRgb(Rgb *color);
        virtual void Copy(IColorSpace *color);
    };

    struct Cmy : public IColorSpace {
        double c, m, y;

        Cmy();
        Cmy(double c, double m, double y);

        virtual void Initialize(Rgb *color);
        virtual void ToRgb(Rgb *color);
        virtual void Copy(IColorSpace *color);
    };

    struct Cmyk : public IColorSpace {
        double c, m, y, k;

        Cmyk();
        Cmyk(double c, double m, double y, double k);

        virtual void Initialize(Rgb *color);
        virtual void ToRgb(Rgb *color);
        virtual void Copy(IColorSpace *color);
    };

    struct Hsv : public IColorSpace {
        double h, s, v;

        Hsv();
        Hsv(double h, double s, double v);

        virtual void Initialize(Rgb *color);
        virtual void ToRgb(Rgb *color);
        virtual void Copy(IColorSpace *color);
    };

    struct Hsb : public IColorSpace {
        double h, s, b;

        Hsb();
        Hsb(double h, double s, double b);

        virtual void Initialize(Rgb *color);
        virtual void ToRgb(Rgb *color);
        virtual void Copy(IColorSpace *color);
    };

    struct HunterLab : public IColorSpace {
        double l, a, b;

        HunterLab();
        HunterLab(double l, double a, double b);

        virtual void Initialize(Rgb *color);
        virtual void ToRgb(Rgb *color);
        virtual void Copy(IColorSpace *color);
    };

    template <typename TColorSpace>
    void IColorSpace::To(TColorSpace *color) {
        Rgb rgb;

        if (typeid(*this) == typeid(*color)) {
            this->Copy(color);
        }
        else {
            this->ToRgb(&rgb);
            IConverter<TColorSpace>::ToColorSpace(&rgb, color);
        }
    }

    Rgb* LerpColorLCH(Rgb &a, Rgb &b, float t);

    std::vector<Rgb> ColorSpectrumLCH(Rgb &a, Rgb &b, size_t n);
    std::vector<Rgb> ColorSpectrumHSV(Rgb &a, Rgb &b, size_t n);
    std::vector<Rgb> ColorSpectrumLUV(Rgb &a, Rgb &b, size_t n);

    std::vector<Rgb> ColorSpectrumRgb(Rgb &a, Rgb &b, size_t n);
}

#endif
