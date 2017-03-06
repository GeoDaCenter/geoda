Name:		GeoDa
Version:	1.8
Release:	1%{?dist}
Summary:	An Introduction to Spatial Data Analysis

Group:		Center for Spatial Data Science
License:	GPL
URL:		http://geodacenter.github.io
Source0:	geoda.tar.gz


BuildRoot: %{_tmppath}/%{name}-buildroot

%description
GeoDa is a free and open source software tool that serves as an introduction to spatial data analysis. It is designed to facilitate new insights from data analysis by exploring and modeling spatial patterns.

%prep
%setup -q


%build

%install
install -m 0766 -d $RPM_BUILD_ROOT/usr/share/applications
install -m 0767 -d $RPM_BUILD_ROOT/usr/local/GeoDa/basemap_cache
install -m 0766 -d $RPM_BUILD_ROOT/usr/local/GeoDa/Resources
install -m 0766 -d $RPM_BUILD_ROOT/usr/local/GeoDa/plugins
install -m 0766 -d $RPM_BUILD_ROOT/usr/local/GeoDa/gdaldata
install -m 0766 -d $RPM_BUILD_ROOT/usr/local/GeoDa/web_plugins
install -m 0755 run.sh $RPM_BUILD_ROOT/usr/local/GeoDa/run.sh
install -m 0755 cache.sqlite $RPM_BUILD_ROOT/usr/local/GeoDa/cache.sqlite
install -m 0755 GeoDa $RPM_BUILD_ROOT/usr/local/GeoDa/GeoDa
install -m 0755 GeoDa.png $RPM_BUILD_ROOT/usr/local/GeoDa/GeoDa.png
install -m 0755 Resources/* $RPM_BUILD_ROOT/usr/local/GeoDa/Resources/
install -m 0755 plugins/* $RPM_BUILD_ROOT/usr/local/GeoDa/plugins/
install -m 0755 gdaldata/* $RPM_BUILD_ROOT/usr/local/GeoDa/gdaldata/
install -m 0757 web_plugins/* $RPM_BUILD_ROOT/usr/local/GeoDa/web_plugins/
install -m 0755 GeoDa.desktop $RPM_BUILD_ROOT/usr/share/applications/GeoDa.desktop


%files
/usr/share/applications/GeoDa.desktop
/usr/local/GeoDa/run.sh
/usr/local/GeoDa/GeoDa
/usr/local/GeoDa/GeoDa.png
/usr/local/GeoDa/Resources/*
/usr/local/GeoDa/plugins/*
/usr/local/GeoDa/gdaldata/*
/usr/local/GeoDa/web_plugins/*
%attr(0767, -, -) /usr/local/GeoDa/basemap_cache/
%attr(0767, -, -) /usr/local/GeoDa/web_plugins/
%attr(0767, -, -) /usr/local/GeoDa/cache.sqlite

%doc



%changelog

