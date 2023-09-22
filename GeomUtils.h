/**
 * GeoDa TM, Copyright (C) 2011-2015 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 *
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GEOMUTILS_H
#define GEOMUTILS_H

#include <ogrsf_frmts.h>

#include <string>
#include <vector>

// read vector file using OGR
std::vector<OGRFeature*> read_vector_file(const std::string& filename);

// convert bytes to hex string
std::string bytes_to_hex(const unsigned char* bytes, size_t n_bytes);

// convert hex to bytes
const size_t hex_to_bytes(const std::string& hex_string, unsigned char** bytes);

// convert geometry to hexewkb string
std::string geometry_to_hexewkb(const OGRGeometry* geom);

// create OGRGeometry from hexewkb string
OGRGeometry* hexewkb_to_geometry(const std::string& hexewkb);

// create OGRFeature from latitude and longitude
OGRFeature* create_ogrfeature_from_latlong(double lat, double lng);

// write OGRLayer to e.g. Arrow file
void save_ogrlayer(OGRLayer* layer, const std::string& filename, const std::string& driver_name = "Arrow",
                   char** options = nullptr);

// convert Geojson to Arrow file
void vector_to_arrow(const std::string& geojson_filename, const std::string& arrow_filename);

// convert Geojson to CSV file
void vector_to_csv(const std::string& geojson_filename, const std::string& csv_filename);

#endif
