// Do GeoDa's own engine and spreg agree?
//
// GeoDa's own engine and spreg agree where they overlap.  The only estimator
// they still share is OLS: the native spatial lag and error models are ML, and
// the solver stopped offering spreg's ML ids, while its remaining lag/error ids
// are GMM/IV - a different estimator, not a parity check.  This runs OLS
// through both engines, on the same data, and puts the numbers side by side:
// the two are written by different people with different optimisers, so what is
// being checked is that they are answering the same question, not that they are
// the same program.
//
// OLS does not depend on W, so this guards the two least-squares paths and the
// job/result format, not the weights; the job still writes them because that is
// what a real job carries and both sides have to parse them.
//
//     spreg_engine/tools/run_parity_test.sh [engine-dir]
//
// It exits non-zero when a number is further apart than the tolerance below,
// which is what makes it usable as a CI gate.

#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/utils.h>

#include "../../GenUtils.h"
#include "../../ShapeOperations/GalWeight.h"
#include "../../Regression/DiagnosticReport.h"
#include "../../Regression/SpregEngine.h"
#include "../../Regression/SpregJob.h"

// the three entry points of GeoDa's own engine, declared where they are defined
// (Regression/smile2.cpp) and used from DialogTools/RegressionDlg.cpp
bool classicalRegression(GalElement *g, int num_obs, double * Y, int dim,
						 double ** X, int expl, DiagnosticReport *dr,
						 bool InclConstant, bool m_moranz, wxGauge* gauge,
						 bool do_white_test);
bool spatialLagRegression(GalElement *g, int num_obs, double * Y, int dim,
						  double ** X, int deps, DiagnosticReport *dr,
						  bool InclConstant, wxGauge* p_bar = 0);
bool spatialErrorRegression(GalElement *g, int num_obs, double * Y, int dim,
							double ** XX, int deps, DiagnosticReport *rr,
							bool InclConstant, wxGauge* p_bar = 0);

// GenUtils, stubbed: this binary is only the two engines and a comparison
static wxString g_shipped_root;
wxString GenUtils::GetResourceDir() { return g_shipped_root + wxFileName::GetPathSeparator(); }
wxString GenUtils::GetExeDir() { return g_shipped_root + wxFileName::GetPathSeparator(); }
wxString GenUtils::GetFileName(const wxString& path)
{
	return wxFileName(path).GetFullName();
}
// only the weights class's saving code reaches this one, which the test never
// calls, but it has to exist for the link
wxString GenUtils::GetFileNameNoExt(const wxString& path)
{
	const wxString name = wxFileName(path).GetFullName();
	const int pos = name.Find('.');
	return pos >= 0 ? name.SubString(0, pos - 1) : name;
}

namespace {

const int ROWS = 6, COLS = 10;
const int N = ROWS * COLS;
const double kCoefficientTolerance = 1e-4;      // absolute, per coefficient
const double kLogLikTolerance = 1e-3;           // relative

int failures = 0;

/** A dataset, its rook contiguity weights, and a line the estimates can be read off. */
struct Data {
	std::vector<double> y, x1, x2;
	std::vector<GalElement> gal;

	Data()
	{
		y.resize(N);
		x1.resize(N);
		x2.resize(N);
		unsigned int seed = 20261007u;
		for (int i = 0; i < N; ++i) {
			seed = seed * 1103515245u + 12345u;
			const double u1 = ((seed >> 16) & 0x7fff) / 32768.0 - 0.5;
			seed = seed * 1103515245u + 12345u;
			const double u2 = ((seed >> 16) & 0x7fff) / 32768.0 - 0.5;
			seed = seed * 1103515245u + 12345u;
			const double e = ((seed >> 16) & 0x7fff) / 32768.0 - 0.5;
			x1[i] = u1;
			x2[i] = u2;
			y[i] = 1.5 + 2.0 * u1 - 0.75 * u2 + 0.25 * e;
		}
		gal.resize(N);
		for (int r = 0; r < ROWS; ++r) {
			for (int c = 0; c < COLS; ++c) {
				const int i = r * COLS + c;
				const int dr[4] = { -1, 1, 0, 0 };
				const int dc[4] = { 0, 0, -1, 1 };
				std::vector<long> neighbours;
				for (int k = 0; k < 4; ++k) {
					const int rr = r + dr[k], cc = c + dc[k];
					if (rr < 0 || rr >= ROWS || cc < 0 || cc >= COLS) continue;
					neighbours.push_back(rr * COLS + cc);
				}
				// the storage has to exist before SetNbr can fill it
				gal[i].SetSizeNbrs(neighbours.size());
				for (size_t k = 0; k < neighbours.size(); ++k) {
					gal[i].SetNbr(k, neighbours[k], 1.0);
				}
			}
		}
	}
};

/** What one engine said about one model. */
struct Estimate {
	std::vector<double> exog;
	double logll, r2;
	bool ok;

	Estimate() : logll(0), r2(0), ok(false) {}
};

void check(const wxString& what, bool ok, const wxString& detail = "")
{
	std::printf("%-52s %s\n", (const char*) what.utf8_str(), ok ? "ok" : "FAIL");
	if (!ok) {
		++failures;
		if (!detail.IsEmpty()) std::printf("     %s\n", (const char*) detail.utf8_str());
	}
}

/** Compares one number from each engine and says how far apart they are. */
void compare(const wxString& what, double native_value, double spreg_value,
			 double tolerance, double scale = 1.0)
{
	const double difference = std::fabs(native_value - spreg_value);
	const bool ok = difference <= tolerance * scale;
	std::printf("  %-22s GeoDa %14.8f   spreg %14.8f   diff %10.3g   %s\n",
				(const char*) what.utf8_str(), native_value, spreg_value, difference,
				ok ? "ok" : "DIFFERS");
	if (!ok) ++failures;
}

/** Runs OLS through GeoDa's own engine. */
bool run_native(Data& data, Estimate& out)
{
	// The engine's DenseVector absorbs what it is handed, so every model gets its
	// own copy of the data - reusing one array would feed the next model a
	// mangled one, which is exactly what a first version of this file did.
	std::vector<std::vector<double> > x(3);
	x[0].assign(N, 1.0);
	x[1] = data.x1;
	x[2] = data.x2;
	double* X[3];
	for (int i = 0; i < 3; ++i) X[i] = &x[i][0];
	std::vector<double> y = data.y;

	DiagnosticReport dr(N, 3, true, true, 1);
	if (!classicalRegression(&data.gal[0], N, &y[0], N, X, 3, &dr, true, true,
							 NULL, false)) {
		return false;
	}
	for (int i = 0; i < 3; ++i) out.exog.push_back(dr.GetCoefficient(i));
	out.logll = dr.GetLIK();
	out.r2 = dr.GetR2();
	out.ok = true;
	return true;
}

/** Runs the same model through the solver, on the same data and weights. */
bool run_spreg(const wxString& engine_dir, const wxString& job_dir, const wxString& model,
			   const Data& data, Estimate& out)
{
	const wxString data_path = job_dir + "/data.bin";
	const wxString result_path = job_dir + "/result.json";
	if (wxFileName::FileExists(data_path)) wxRemoveFile(data_path);
	if (wxFileName::FileExists(result_path)) wxRemoveFile(result_path);

	SpregJob::Writer writer(job_dir);
	std::vector<std::vector<double> > x(2);
	x[0] = data.x1;
	x[1] = data.x2;
	std::vector<wxString> x_names;
	x_names.push_back("x1");
	x_names.push_back("x2");
	wxString err;
	if (!writer.AddY(data.y, err) || !writer.AddX(x, err)
		|| !writer.AddWeights(&data.gal[0], N, err)) {
		std::printf("     writing the job failed: %s\n", (const char*) err.utf8_str());
		return false;
	}
	// no options at all: the registry's defaults are what the dialog would send
	std::map<wxString, wxString> options;
	// the job carries no constant column: the solver adds it
	if (!writer.Write(model, options, "y", x_names, "grid-rook", "",
					  std::vector<wxString>(), std::vector<wxString>(),
					  std::vector<wxString>(), err)) {
		std::printf("     writing job.json failed: %s\n", (const char*) err.utf8_str());
		return false;
	}

	wxString output, run_err;
	if (SpregEngine::RunJob(engine_dir, job_dir, output, run_err, 300) != 0) {
		std::printf("     the solver failed: %s\n", (const char*) run_err.utf8_str());
		return false;
	}
	SpregJob::Result result;
	if (!result.Read(job_dir, err) || !result.ok) {
		const wxString why = result.error_message.IsEmpty() ? err : result.error_message;
		std::printf("     reading the result failed: %s\n", (const char*) why.utf8_str());
		return false;
	}
	for (size_t i = 0; i < result.names.size(); ++i) {
		const wxString role = i < result.roles.size() ? result.roles[i] : "exog";
		// OLS puts every coefficient in "exog"; a lag/error role would belong to
		// a model this check no longer runs.
		if (role == "exog") out.exog.push_back(result.estimate[i]);
	}
	std::map<wxString, double>::const_iterator ll = result.fit.find("logll");
	if (ll != result.fit.end()) out.logll = ll->second;
	std::map<wxString, double>::const_iterator r2 = result.fit.find("r2");
	if (r2 == result.fit.end()) r2 = result.fit.find("pr2");
	if (r2 != result.fit.end()) out.r2 = r2->second;
	out.ok = true;
	return true;
}

/** Both engines, OLS, and the comparison. */
void compare_model(const wxString& engine_dir, const wxString& job_dir, Data& data,
				   const wxString& model)
{
	std::printf("\n%s\n", (const char*) model.utf8_str());
	Estimate native, spreg;
	if (!run_native(data, native)) {
		check("GeoDa's own engine runs", false, "it refused the data");
		return;
	}
	if (!run_spreg(engine_dir, job_dir, model, data, spreg)) {
		check("the solver runs", false);
		return;
	}
	for (size_t i = 0; i < native.exog.size() && i < spreg.exog.size(); ++i) {
		compare(wxString::Format("coefficient %d", (int) i), native.exog[i], spreg.exog[i],
				kCoefficientTolerance);
	}
	compare("log likelihood", native.logll, spreg.logll, kLogLikTolerance,
			std::fabs(native.logll));
}

} // namespace

int main(int argc, char** argv)
{
	setvbuf(stdout, NULL, _IONBF, 0);          // so a crash does not hide the output
	wxInitializer initializer(argc, argv);
	if (argc < 3) {
		std::printf("usage: %s <engine-dir> <scratch-dir> [repo-root]\n", argv[0]);
		return 2;
	}
	const wxString engine_dir = wxString::FromUTF8(argv[1]);
	const wxString scratch = wxString::FromUTF8(argv[2]);
	// the solver the application ships lives in the repository's spreg_engine
	g_shipped_root = argc > 3 ? wxString::FromUTF8(argv[3]) : wxGetCwd();
	const wxString job_dir = scratch + "/parity-job";
	wxFileName::Mkdir(job_dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

	std::printf("building the data and the weights\n");
	Data data;
	std::printf("  %d observations; a corner has %ld neighbours and the lag of a "
				"constant is %g\n", N, data.gal[0].Size(),
				data.gal[0].SpatialLag(std::vector<double>(N, 1.0), false));

	compare_model(engine_dir, job_dir, data, "OLS");
	// The ML lag and error comparisons that used to be here went with the ML
	// model ids.  RunJob always uses the solver shipped with the application
	// (Regression/SpregEngine.cpp, SolverScriptPath), not one inside the
	// downloaded archive - the archive carries no solver - so asking it for
	// ML_Lag would now fail with `unknown model`.  GM_Lag is not a substitute:
	// it is GMM/2SLS and answers a different estimation question from GeoDa's
	// native ML lag.  OLS is therefore the only comparison left, and it does not
	// exercise the weights; if weight handling needs a gate, compare the LM /
	// Moran diagnostics both engines compute for OLS.

	std::printf("\n%s\n", failures ? "FAILED" : "the two engines agree");
	return failures ? 1 : 0;
}
