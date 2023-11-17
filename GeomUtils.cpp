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
#include "GeomUtils.h"

#include <iostream>

// read vector file using OGR
std::vector<OGRFeature*> read_vector_file(const std::string& filename) {
  std::vector<OGRFeature*> data;
  GDALAllRegister();
  GDALDataset* ds = GDALDataset::Open(filename.c_str(), GDAL_OF_VECTOR);
  if (ds == nullptr) {
    return data;
  }
  OGRLayer* layer = ds->GetLayer(0);
  if (layer == nullptr) {
    return data;
  }
  layer->ResetReading();
  OGRFeature* feat = nullptr;
  while ((feat = layer->GetNextFeature()) != nullptr) {
    data.push_back(feat);
  }
  return data;
}

// convert bytes to hex string
std::string bytes_to_hex(const unsigned char* bytes, size_t n_bytes) {
  static const char* hex_digits = "0123456789ABCDEF";
  std::string hex_string;
  for (size_t i = 0; i < n_bytes; ++i) {
    const unsigned char c = bytes[i];
    hex_string.push_back(hex_digits[c >> 4]);
    hex_string.push_back(hex_digits[c & 15]);
  }
  return hex_string;
}

// convert hex to bytes
const size_t hex_to_bytes(const std::string& hex_string, unsigned char** bytes) {
  const size_t n_bytes = hex_string.size() / 2;
  *bytes = new unsigned char[n_bytes];
  for (size_t i = 0; i < n_bytes; ++i) {
    char c1 = hex_string[2 * i];
    char c2 = hex_string[2 * i + 1];
    c1 = c1 >= 'A' ? c1 - 'A' + 10 : c1 - '0';
    c2 = c2 >= 'A' ? c2 - 'A' + 10 : c2 - '0';
    (*bytes)[i] = (c1 << 4) | c2;
  }
  return n_bytes;
}

// check if hexewkb string is valid
bool valid_hexewkb(const std::string& hexewkb) {
  unsigned char* bytes;
  const size_t n_bytes = hex_to_bytes(hexewkb, &bytes);
  OGRGeometry* geom = nullptr;
  OGRErr err = OGRGeometryFactory::createFromWkb(bytes, nullptr, &geom, n_bytes);
  delete[] bytes;
  if (err != OGRERR_NONE) {
    return false;
  }
  delete geom;
  return true;
}

// convert geometry to hexewkb string
std::string geometry_to_hexewkb(const OGRGeometry* geom) {
  int nBLOBLen = geom->WkbSize();
  GByte* pabyGeomBLOB = reinterpret_cast<GByte*>(VSIMalloc(nBLOBLen));
  geom->exportToWkb(wkbNDR, pabyGeomBLOB);
  // hex-encoded WKB strings
  return bytes_to_hex(pabyGeomBLOB, nBLOBLen);
}

// create OGRGeometry from hexewkb string
OGRGeometry* hexewkb_to_geometry(const std::string& hexewkb) {
  unsigned char* bytes;
  const size_t n_bytes = hex_to_bytes(hexewkb, &bytes);
  OGRGeometry* geom = nullptr;
  OGRGeometryFactory::createFromWkb(bytes, nullptr, &geom, n_bytes);
  delete[] bytes;
  return geom;
}

// create OGRFeature from latitude and longitude
OGRFeature* create_ogrfeature_from_latlong(double lat, double lng) {
  // create OGRFeatureDefn for point
  OGRFeatureDefn* feat_defn = new OGRFeatureDefn("point");
  OGRFeature* feat = OGRFeature::CreateFeature(feat_defn);
  OGRPoint* point = new OGRPoint(lng, lat);
  feat->SetGeometryDirectly(point);
  return feat;
}

// write OGRLayer to Arrow file
void save_ogrlayer(OGRLayer* layer, const std::string& filename, const std::string& driver_name, char** options) {
  GDALAllRegister();
  GDALDriver* driver = GetGDALDriverManager()->GetDriverByName(driver_name.c_str());
  if (driver == nullptr) {
    return;
  }
  GDALDataset* ds = driver->Create(filename.c_str(), 0, 0, 0, GDT_Unknown, nullptr);
  if (ds == nullptr) {
    return;
  }
  // validate geometry type by looping over features in layer
  OGRwkbGeometryType geom_type = layer->GetGeomType();
  // layer->ResetReading();
  OGRFeature* feat = nullptr;
  // while ((feat = layer->GetNextFeature()) != nullptr) {
  //   const OGRGeometry* geom = feat->GetGeometryRef();
  //   if (geom != nullptr) {
  //     OGRwkbGeometryType feat_geom_type = geom->getGeometryType();
  //     if (feat_geom_type == wkbMultiPoint || feat_geom_type == wkbMultiPolygon ||
  //         feat_geom_type == wkbMultiLineString) {
  //       geom_type = feat_geom_type;
  //       break;
  //     }
  //   }
  // }
  OGRLayer* new_layer = ds->CreateLayer(layer->GetName(), layer->GetSpatialRef(), geom_type, options);
  if (new_layer == nullptr) {
    return;
  }

  // iterate over fields in layer
  for (unsigned int i = 0; i < layer->GetLayerDefn()->GetFieldCount(); ++i) {
    OGRFieldDefn* field = layer->GetLayerDefn()->GetFieldDefn(i);
    new_layer->CreateField(field);
  }

  layer->ResetReading();
  while ((feat = layer->GetNextFeature()) != nullptr) {
    // const OGRGeometry* geom = feat->GetGeometryRef();
    // if (geom != nullptr) {
    //   if (geom->getGeometryType() == wkbPolygon && geom_type == wkbMultiPolygon) {
    //     OGRMultiPolygon* multigeom = new OGRMultiPolygon();
    //     multigeom->addGeometry(geom);
    //     feat->SetGeometryDirectly(multigeom);
    //   } else if (geom->getGeometryType() == wkbLineString && geom_type == wkbMultiLineString) {
    //     OGRMultiLineString* multigeom = new OGRMultiLineString();
    //     multigeom->addGeometry(geom);
    //     feat->SetGeometryDirectly(multigeom);
    //   } else if (geom->getGeometryType() == wkbPoint && geom_type == wkbMultiPoint) {
    //     OGRMultiPoint* multigeom = new OGRMultiPoint();
    //     multigeom->addGeometry(geom);
    //     feat->SetGeometryDirectly(multigeom);
    //   }
    // }
    if (new_layer->CreateFeature(feat) != OGRERR_NONE) {
      return;
    }
  }
  ds->FlushCache();
  GDALClose(ds);
}

// convert to Arrow file
void vector_to_arrow(const std::string& geojson_filename, const std::string& arrow_filename) {
  GDALAllRegister();
  GDALDataset* ds = GDALDataset::Open(geojson_filename.c_str(), GDAL_OF_VECTOR);
  if (ds == nullptr) {
    return;
  }
  OGRLayer* layer = ds->GetLayer(0);
  if (layer == nullptr) {
    return;
  }
  char** options = nullptr;
  options = CSLSetNameValue(options, "COMPRESSION", "NONE");
  // options = CSLSetNameValue(options, "GEOMETRY_ENCODING", "WKB");

  save_ogrlayer(layer, arrow_filename, "Arrow", options);
  CSLDestroy(options);
}

// convert to CSV file
void vector_to_csv(const std::string& geojson_filename, const std::string& csv_filename) {
  GDALAllRegister();
  GDALDataset* ds = GDALDataset::Open(geojson_filename.c_str(), GDAL_OF_VECTOR);
  if (ds == nullptr) {
    return;
  }
  OGRLayer* layer = ds->GetLayer(0);
  if (layer == nullptr) {
    return;
  }
  char** options = nullptr;
  options = CSLSetNameValue(options, "GEOMETRY", "AS_WKB");

  save_ogrlayer(layer, csv_filename, "CSV", options);

  CSLDestroy(options);
}

// compute mean center of geometry
void mean_center(const OGRGeometry* geom, OGRPoint* point) {
  double x = 0;
  double y = 0;
  double n = 0;
  // get geometry type
  const OGRwkbGeometryType geom_type = geom->getGeometryType();
  if (geom_type == wkbPoint) {
    const OGRPoint* pt = dynamic_cast<const OGRPoint*>(geom);
    x = pt->getX();
    y = pt->getY();
    ++n;
  } else if (geom_type == wkbMultiPoint) {
    const OGRMultiPoint* input_multipoint = dynamic_cast<const OGRMultiPoint*>(geom);
    for (int i = 0; i < input_multipoint->getNumGeometries(); ++i) {
      const OGRPoint* pt = dynamic_cast<const OGRPoint*>(input_multipoint->getGeometryRef(i));
      x += pt->getX();
      y += pt->getY();
      ++n;
    }
  } else if (geom_type == wkbPolygon) {
    const OGRPolygon* input_polygon = dynamic_cast<const OGRPolygon*>(geom);
    for (int i = 0; i < input_polygon->getNumInteriorRings() + 1; ++i) {
      const OGRLinearRing* input_ring =
          i == 0 ? input_polygon->getExteriorRing() : input_polygon->getInteriorRing(i - 1);
      const auto& pt_it = input_ring->getPointIterator();
      OGRPoint pt;
      while (pt_it->getNextPoint(&pt)) {
        x += pt.getX();
        y += pt.getY();
        ++n;
      }
    }
  } else if (geom_type == wkbMultiPolygon) {
    const OGRMultiPolygon* input_multipolygon = dynamic_cast<const OGRMultiPolygon*>(geom);
    for (int i = 0; i < input_multipolygon->getNumGeometries(); ++i) {
      const OGRPolygon* input_polygon = dynamic_cast<const OGRPolygon*>(input_multipolygon->getGeometryRef(i));
      for (int j = 0; j < input_polygon->getNumInteriorRings() + 1; ++j) {
        const OGRLinearRing* input_ring =
            j == 0 ? input_polygon->getExteriorRing() : input_polygon->getInteriorRing(j - 1);
        const auto& pt_it = input_ring->getPointIterator();
        OGRPoint pt;
        while (pt_it->getNextPoint(&pt)) {
          x += pt.getX();
          y += pt.getY();
          ++n;
        }
      }
    }
  } else {
    // print error message
    std::cout << "Error: invalid geometry type" << std::endl;
  }
  point->setX(x / n);
  point->setY(y / n);
}

// compute center of mass or center of gravity of geometry
void center_of_mass(const OGRGeometry* geom, OGRPoint* point) {
  const OGRwkbGeometryType geom_type = geom->getGeometryType();

  if (geom_type == wkbPoint) {
    const OGRPoint* pt = dynamic_cast<const OGRPoint*>(geom);
    point->setX(pt->getX());
    point->setY(pt->getY());
  } else if (geom->getGeometryType() == wkbPolygon) {
    std::vector<OGRPoint*> neutralized_points;

    OGRPoint translation;
    mean_center(geom, &translation);

    const OGRPolygon* input_polygon = dynamic_cast<const OGRPolygon*>(geom);
    for (int i = 0; i < input_polygon->getNumInteriorRings() + 1; ++i) {
      const OGRLinearRing* input_ring =
          i == 0 ? input_polygon->getExteriorRing() : input_polygon->getInteriorRing(i - 1);
      const auto& pt_it = input_ring->getPointIterator();
      OGRPoint* pt = new OGRPoint();
      while (pt_it->getNextPoint(pt)) {
        pt->setX(pt->getX() - translation.getX());
        pt->setY(pt->getY() - translation.getY());
        neutralized_points.push_back(pt);
      }
    }

    double sx = 0;
    double sy = 0;
    double s_area = 0;

    for (size_t i = 0; i < neutralized_points.size(); ++i) {
      const OGRPoint* pt = neutralized_points[i];
      const OGRPoint* next_pt = neutralized_points[(i + 1) % neutralized_points.size()];
      const double area = pt->getX() * next_pt->getY() - next_pt->getX() * pt->getY();
      sx += (pt->getX() + next_pt->getX()) * area;
      sy += (pt->getY() + next_pt->getY()) * area;
      s_area += area;
    }

    if (s_area == 0) {
      point->setX(translation.getX());
      point->setY(translation.getY());
    } else {
      point->setX(sx / (3 * s_area) + translation.getX());
      point->setY(sy / (3 * s_area) + translation.getY());
    }
  } else {
    OGRGeometry* convex_hull = geom->ConvexHull();

    if (convex_hull) {
      center_of_mass(convex_hull, point);
      delete convex_hull;
    } else {
      mean_center(geom, point);
    }
  }
}
