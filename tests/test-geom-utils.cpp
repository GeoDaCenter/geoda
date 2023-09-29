#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <ogrsf_frmts.h>
#include <wx/filesys.h>
#include <wx/fs_mem.h>
#include <wx/textfile.h>

#include <vector>

#include "GeomUtils.h"

namespace {
// test case: hex to bytes
TEST(GEOM_UTILS_TEST, TEST_HEX_TO_BYTES) {
  const std::string hex_string = "0101000000000000000000F03F000000000000F03F";
  unsigned char* bytes;
  const size_t n_bytes = hex_to_bytes(hex_string, &bytes);
  EXPECT_EQ(n_bytes, 21);

  const std::string expect_hex_string = bytes_to_hex(bytes, n_bytes);
  EXPECT_EQ(hex_string, expect_hex_string);

  delete[] bytes;
}

// test case: convert Geojson to Arrow file
TEST(GEOM_UTILS_TEST, TEST_GEOJSON_TO_ARROW) {
  const std::string geojson_filename = "./data/guerry.geojson";
  const std::string arrow_filename = "/vsimem/guerry.arrow";
  vector_to_arrow(geojson_filename, arrow_filename);

  const std::vector<OGRFeature*> data = read_vector_file(arrow_filename);

  // read vsimem file in char* buffer
  VSILFILE* fp = VSIFOpenL(arrow_filename.c_str(), "rb");
  VSIFSeekL(fp, 0, SEEK_END);
  const size_t size = VSIFTellL(fp);
  VSIFSeekL(fp, 0, SEEK_SET);
  char* buffer = new char[size];
  VSIFReadL(buffer, 1, size, fp);
  VSIFCloseL(fp);

  // print size
  std::cout << "size: " << size << std::endl;
  EXPECT_EQ(data.size(), 85);
}

// test case: convert Parquet to Arrow file
// TEST(GEOM_UTILS_TEST, TEST_PARQUET_TO_ARROW) {
//   const std::string geojson_filename = "./data/Utah.parquet";
//   const std::string arrow_filename = "./data/Utah.arrow";
//   vector_to_arrow(geojson_filename, arrow_filename);

//   const std::vector<OGRFeature*> data = read_vector_file(arrow_filename);
//   EXPECT_EQ(data.size(), 85);
// }

// test case: convert Geojson to CSV file
TEST(GEOM_UTILS_TEST, TEST_GEOJSON_TO_CSV) {
  const std::string geojson_filename = "./data/guerry.geojson";
  const std::string csv_filename = "./data/geojson.csv";
  vector_to_csv(geojson_filename, csv_filename);

  const std::vector<OGRFeature*> data = read_vector_file(csv_filename);
  EXPECT_EQ(data.size(), 85);
}

// test case: create OGRFeature from latitude and longitude and convert it to hexewkb string, verify the hexewkb string
TEST(GEOM_UTILS_TEST, TEST_HEXEWKB) {
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
TEST(GEOM_UTILS_TEST, TEST_READ_GEOJSON) {
  const std::string filename = "./data/guerry.geojson";
  const std::vector<OGRFeature*> data = read_vector_file(filename);
  EXPECT_EQ(data.size(), 85);
}
}  // namespace
