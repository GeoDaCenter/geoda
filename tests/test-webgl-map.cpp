#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <ogrsf_frmts.h>
#include <wx/filesys.h>
#include <wx/fs_mem.h>
#include <wx/textfile.h>

#include <vector>

using namespace testing;

namespace {
TEST(WEBGL_MAP_TEST, TEST_OGRLAYER_TO_CSV) {
  // create a CSV file contains the wkt geometry and selected variables
  const wxString csv_filename = "webgl_map.csv";
  wxFileSystem::AddHandler(new wxMemoryFSHandler);
  wxMemoryFSHandler::AddFile(csv_filename, wxEmptyString);

  wxTextFile csv_file("memory:" + csv_filename);
  const wxString first_line = "id, geom";
  csv_file.AddLine(first_line);

  const std::vector<OGRFeature*> data;
  const size_t n_rows = data.size();

  for (size_t i = 0; i < n_rows; ++i) {
    const OGRFeature* feat = data[i];
    const OGRGeometry* geom = feat->GetGeometryRef();
    std::string wkt = geom->exportToWkt();
    csv_file.AddLine(wxString::Format("%d, %s", i, wkt));
  }


  EXPECT_EQ(csv_file.GetFirstLine(), first_line);
  EXPECT_EQ(csv_file.GetLineCount(), n_rows);

  csv_file.Close();
  wxMemoryFSHandler::RemoveFile(csv_filename);
}
}  // namespace
