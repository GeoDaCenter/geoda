#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <ogrsf_frmts.h>
#include <wx/filesys.h>
#include <wx/fs_mem.h>
#include <wx/textfile.h>

#include <vector>

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
void ogrlayer_to_arrow(OGRLayer* layer, const std::string& filename) {
  GDALAllRegister();
  GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("Arrow");
  if (driver == nullptr) {
    return;
  }
  GDALDataset* ds = driver->Create(filename.c_str(), 0, 0, 0, GDT_Unknown, nullptr);
  if (ds == nullptr) {
    return;
  }
  OGRLayer* new_layer = ds->CreateLayer(layer->GetName(), layer->GetSpatialRef(), layer->GetGeomType(), nullptr);
  if (new_layer == nullptr) {
    return;
  }

  // iterate over fields in layer
  for (unsigned int i = 0; i < layer->GetLayerDefn()->GetFieldCount(); ++i) {
    OGRFieldDefn* field = layer->GetLayerDefn()->GetFieldDefn(i);
    new_layer->CreateField(field);
  }

  layer->ResetReading();
  OGRFeature* feat = nullptr;
  while ((feat = layer->GetNextFeature()) != nullptr) {
    if (new_layer->CreateFeature(feat) != OGRERR_NONE) {
      return;
    }
  }
  ds->FlushCache();
  GDALClose(ds);
}

// convert Geojson to Arrow file
void geojson_to_arrow(const std::string& geojson_filename, const std::string& arrow_filename) {
  GDALAllRegister();
  GDALDataset* ds = GDALDataset::Open(geojson_filename.c_str(), GDAL_OF_VECTOR);
  if (ds == nullptr) {
    return;
  }
  OGRLayer* layer = ds->GetLayer(0);
  if (layer == nullptr) {
    return;
  }
  ogrlayer_to_arrow(layer, arrow_filename);
}

namespace {
// test case: hex to bytes
TEST(WEBGL_MAP_TEST, TEST_HEX_TO_BYTES) {
  const std::string hex_string = "0101000000000000000000F03F000000000000F03F";
  unsigned char* bytes;
  const size_t n_bytes = hex_to_bytes(hex_string, &bytes);
  EXPECT_EQ(n_bytes, 21);

  const std::string expect_hex_string = bytes_to_hex(bytes, n_bytes);
  EXPECT_EQ(hex_string, expect_hex_string);

  delete[] bytes;
}

// test case: convert Geojson to Arrow file
TEST(WEBGL_MAP_TEST, TEST_GEOJSON_TO_ARROW) {
  const std::string geojson_filename = "./data/guerry.geojson";
  const std::string arrow_filename = "./data/guerry.arrow";
  geojson_to_arrow(geojson_filename, arrow_filename);

  const std::vector<OGRFeature*> data = read_vector_file(arrow_filename);
  EXPECT_EQ(data.size(), 85);
}

// test case: create OGRFeature from latitude and longitude and convert it to hexewkb string, verify the hexewkb string
TEST(WEBGL_MAP_TEST, TEST_HEXEWKB) {
  OGRFeature* feat = create_ogrfeature_from_latlong(1, 1);
  const OGRGeometry* geom = feat->GetGeometryRef();
  const std::string hexewkb = geometry_to_hexewkb(geom);
  OGRFeature::DestroyFeature(feat);
  EXPECT_EQ(hexewkb, "0101000000000000000000F03F000000000000F03F");

  OGRGeometry* new_geom = hexewkb_to_geometry(hexewkb);
  EXPECT_EQ(new_geom->getGeometryType(), wkbPoint);

  // convert OGRGeometry to OGRPoint, test if the coordinates are correct
  OGRPoint* new_point = reinterpret_cast<OGRPoint*>(new_geom);
  EXPECT_EQ(new_point->getX(), 1);
  EXPECT_EQ(new_point->getY(), 1);

  delete new_geom;
}

// test case: read Geojson using OGR
TEST(WEBGL_MAP_TEST, TEST_READ_GEOJSON) {
  const std::string filename = "./data/guerry.geojson";
  const std::vector<OGRFeature*> data = read_vector_file(filename);
  EXPECT_EQ(data.size(), 85);
}

// test case: create a CSV file contains the wkt geometry and selected variables
TEST(WEBGL_MAP_TEST, TEST_OGRLAYER_TO_CSV_WKT) {
  // create a CSV file contains the wkt geometry and selected variables
  const wxString csv_filename = "webgl_map.csv";
  wxFileSystem::AddHandler(new wxMemoryFSHandler);
  wxMemoryFSHandler::AddFile(csv_filename, wxEmptyString);

  wxTextFile csv_file("memory:" + csv_filename);
  const wxString first_line = "id, geom";
  csv_file.AddLine(first_line);

  const std::vector<OGRFeature*> data = read_vector_file("./data/guerry.geojson");
  const size_t n_rows = data.size();

  for (size_t i = 0; i < n_rows; ++i) {
    const OGRFeature* feat = data[i];
    const OGRGeometry* geom = feat->GetGeometryRef();
    std::string wkt = geom->exportToWkt();
    csv_file.AddLine(wxString::Format("%d, %s", i, wkt));
  }

  EXPECT_EQ(csv_file.GetFirstLine(), first_line);
  EXPECT_EQ(csv_file.GetLineCount(), n_rows + 1);

  csv_file.Close();
  wxMemoryFSHandler::RemoveFile(csv_filename);
}
}  // namespace
