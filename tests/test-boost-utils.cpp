#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <ogrsf_frmts.h>
#include <wx/filesys.h>
#include <wx/fs_mem.h>
#include <wx/textfile.h>

#include <boost/geometry.hpp>
#include <boost/geometry/strategies/cartesian/centroid_average.hpp>
#include <boost/geometry/strategies/cartesian/centroid_bashein_detmer.hpp>
#include <vector>

#include "GeomUtils.h"

typedef boost::geometry::model::d2::point_xy<double> point_type;
typedef boost::geometry::model::polygon<point_type> polygon_type;
typedef boost::geometry::model::multi_polygon<polygon_type> multi_polygon_type;

namespace {
// test case: boost centroids
TEST(BOOST_UTILS_TEST, TEST_CENTROIDS) {
  const std::string filename = "./data/natregimes.geojson";
  const std::vector<OGRFeature*> data = read_vector_file(filename);
  EXPECT_EQ(data.size(), 1081586);

  std::vector<polygon_type> polygons;
  std::vector<multi_polygon_type> multi_polygons;
  std::vector<OGRGeometry*> geometries;

  // create boost geometries
  for (size_t i = 0; i < data.size(); ++i) {
    const auto& feat = data[i];
    OGRGeometry* geom = feat->GetGeometryRef();
    geometries.push_back(geom);
    // get wkt from geometry
    char* wkt;
    geom->exportToWkt(&wkt);

    // if geom is OGRPolygon, create boost polygon
    if (geom->getGeometryType() == wkbPolygon) {
      polygon_type boost_poly;
      boost::geometry::read_wkt(wkt, boost_poly);
      polygons.push_back(boost_poly);
    } else if (geom->getGeometryType() == wkbMultiPolygon) {
      multi_polygon_type boost_multipoly;
      boost::geometry::read_wkt(wkt, boost_multipoly);
      multi_polygons.push_back(boost_multipoly);
    }

    // clean up wkt
    CPLFree(wkt);
  }

  // mean centers
  auto start2 = std::chrono::high_resolution_clock::now();
  std::vector<OGRPoint> ogr_mean_centers(data.size());
  for (size_t i = 0; i < geometries.size(); ++i) {
    OGRGeometry* geom = geometries[i];
    mean_center(geom, &ogr_mean_centers[i]);
  }
  auto end2 = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed2 = end2 - start2;
  std::cout << "mean centers took " << elapsed2.count() << " seconds\n";
  std::cout << "centers size: " << ogr_mean_centers.size() << "\n";
  std::cout << "first center" << ogr_mean_centers[0].getX() << std::endl;
  std::cout << "last center" << ogr_mean_centers[geometries.size() - 1].getX() << std::endl;

  // c++ centroids
  auto start3 = std::chrono::high_resolution_clock::now();
  std::vector<OGRPoint> my_centroids(data.size());
  for (size_t i = 0; i < geometries.size(); ++i) {
    OGRGeometry* geom = geometries[i];
    center_of_mass(geom, &my_centroids[i]);
  }
  auto end3 = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed3 = end3 - start3;
  std::cout << "c++ centroids took " << elapsed3.count() << " seconds\n";
  std::cout << "centroids size: " << my_centroids.size() << "\n";
  std::cout << "first centroid" << my_centroids[0].getX() << std::endl;
  std::cout << "last centroid" << my_centroids[geometries.size() - 1].getX() << std::endl;

  // boost centroids
  auto start = std::chrono::high_resolution_clock::now();
  // calculate centroids
  std::vector<point_type> centroids;
  for (const auto& poly : polygons) {
    point_type centroid;
    boost::geometry::centroid(poly, centroid, boost::geometry::strategy::centroid::bashein_detmer());
    centroids.push_back(centroid);
  }
  for (const auto& multipoly : multi_polygons) {
    point_type centroid;
    boost::geometry::centroid(multipoly, centroid, boost::geometry::strategy::centroid::bashein_detmer());
    centroids.push_back(centroid);
  }
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  std::cout << "boost centroids took " << elapsed.count() << " seconds\n";
  std::cout << "centroids size: " << centroids.size() << "\n";
  std::cout << "first centroid" << centroids[0].x() << std::endl;
  std::cout << "last centroid" << centroids[centroids.size() - 1].x() << std::endl;

  // OGR centroids
  std::vector<OGRPoint> ogr_centroids(data.size());
  auto start1 = std::chrono::high_resolution_clock::now();
  // iterator over data
  OGRPoint point;
  for (size_t i = 0; i < geometries.size(); ++i) {
    // get centroid
    auto& geom = geometries[i];
    geom->Centroid(ogr_centroids.data() + i);
  }
  auto end1 = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed1 = end1 - start1;
  std::cout << "ogr centroids took " << elapsed1.count() << " seconds\n";
  std::cout << "centroids size: " << ogr_centroids.size() << "\n";
  std::cout << "first centroid" << ogr_centroids[0].getX() << std::endl;
  std::cout << "last centroid" << ogr_centroids[geometries.size() - 1].getX() << std::endl;
}

}  // namespace
