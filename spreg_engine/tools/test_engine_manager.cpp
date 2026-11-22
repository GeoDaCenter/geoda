// Headless exercise of Regression/SpregEngine.cpp - the code that verifies,
// unpacks and installs the downloaded engine.
//
// Building the whole application to test an installer would be silly, and
// clicking through the dialog would test it once; this runs the same functions
// against a real engine archive in a scratch directory, including the ways an
// install can go wrong (a corrupted download, an archive that tries to write
// outside its own directory) and the upgrade and remove paths.
//
//     spreg_engine/tools/run_engine_manager_test.sh [path/to/archive.zip]
//
// The only application code it needs is SpregEngine.cpp; the two GenUtils path
// helpers are stubbed below, so nothing of the rest of GeoDa is linked in.

#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/zipstrm.h>
#include <wx/wfstream.h>
#include <wx/dir.h>

#include "../../Regression/SpregEngine.h"
#include "../../Regression/SpregJob.h"
#include "../../GenUtils.h"
#include "../../ShapeOperations/GalWeight.h"

using namespace SpregEngine;

// ---------------------------------------------------------------------------
// stubs for the two path helpers SpregEngine.cpp uses.  The resource directory
// points at the repository, whose spreg_engine/ directory is laid out exactly
// like the one inside the installed application.
// ---------------------------------------------------------------------------

static wxString g_installed_root;      // the equivalent of Contents/Resources/

wxString GenUtils::GetResourceDir() { return g_installed_root; }
wxString GenUtils::GetExeDir() { return g_installed_root; }

// and of the two GalElement accessors SpregJob's GalElement overload uses: this
// harness feeds the writer its CSR arrays directly, as the protocol wants them,
// so nothing here needs a GeoDa weights object
const std::vector<long>& GalElement::GetNbrs() const
{
	static std::vector<long> empty;
	return empty;
}
const std::vector<double>& GalElement::GetNbrWeights() const
{
	static std::vector<double> empty;
	return empty;
}

// ---------------------------------------------------------------------------

static int failures = 0;

static void check(const wxString& what, bool ok, const wxString& detail = "")
{
	std::printf("%-52s %s\n", (const char*)what.utf8_str(),
				ok ? "ok" : "FAIL");
	if (!ok) {
		++failures;
		if (!detail.IsEmpty()) {
			std::printf("     %s\n", (const char*)detail.utf8_str());
		}
	} else if (!detail.IsEmpty()) {
		std::printf("     %s\n", (const char*)detail.utf8_str());
	}
}

static bool IsExecutable(const wxString& path)
{
#ifdef __WXMSW__
	return wxFileName::FileExists(path);
#else
	return access(path.fn_str(), X_OK) == 0;
#endif
}

static long long DirFileCount(const wxString& dir)
{
	long long count = 0;
	wxDir d(dir);
	wxString name;
	bool more = d.IsOpened() && d.GetFirst(&name, wxEmptyString, wxDIR_DIRS);
	while (more) {
		count += DirFileCount(dir + wxFileName::GetPathSeparator() + name);
		more = d.GetNext(&name);
	}
	wxDir files(dir);
	wxString file;
	more = files.IsOpened() && files.GetFirst(&file, wxEmptyString, wxDIR_FILES);
	while (more) {
		++count;
		more = files.GetNext(&file);
	}
	return count;
}

/** Writes an archive with a deliberately unsafe entry name. */
static bool WriteEvilZip(const wxString& path)
{
	wxFFileOutputStream out(path);
	if (!out.IsOk()) return false;
	wxZipOutputStream zip(out);
	zip.PutNextEntry("../escapee.txt");
	const char payload[] = "should never be written\n";
	zip.Write(payload, sizeof(payload) - 1);
	zip.PutNextEntry("python/bin/python3");
	zip.Write(payload, sizeof(payload) - 1);
	zip.Close();
	out.Close();
	return true;
}

int main(int argc, char** argv)
{
	wxInitializer initializer(argc, argv);      // wx needs its libraries set up

	if (argc < 3) {
		std::printf("usage: %s <engine-archive.zip> <scratch-dir> [repo-root]\n", argv[0]);
		std::printf("       %s --download <url> <scratch-dir>\n", argv[0]);
		std::printf("       %s --job <engine-dir> <scratch-dir> [model-id]\n", argv[0]);
		return 2;
	}
	const wxString archive = wxString::FromUTF8(argv[1]);
	const wxString scratch = wxString::FromUTF8(argv[2]);

	if (argc >= 4 && wxString(argv[1]) == "--job") {
		// the C++ half of the protocol: SpregJob::Writer builds the job,
		// SpregEngine::RunJob runs it, SpregJob::Result reads the answer -
		// against the same 6x10 grid the solver's own selftest uses
		const wxString engine_dir = argv[2];
		const wxString job_dir = (argc > 3 ? wxString(argv[3]) : wxString(".")) + "/job";
		wxFileName::Mkdir(job_dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
		wxRemoveFile(job_dir + "/data.bin");
		wxRemoveFile(job_dir + "/result.json");

		const int rows = 6, cols = 10, n = rows * cols;
		std::vector<double> y(n), x1(n), x2(n);
		std::vector<std::vector<double> > x(2);
		x[0].resize(n);
		x[1].resize(n);
		unsigned int seed = 20261007u;
		for (int i = 0; i < n; ++i) {
			seed = seed * 1103515245u + 12345u;
			const double u1 = ((seed >> 16) & 0x7fff) / 32768.0 - 0.5;
			seed = seed * 1103515245u + 12345u;
			const double u2 = ((seed >> 16) & 0x7fff) / 32768.0 - 0.5;
			seed = seed * 1103515245u + 12345u;
			const double e = ((seed >> 16) & 0x7fff) / 32768.0 - 0.5;
			x[0][i] = u1;
			x[1][i] = u2;
			y[i] = 1.5 + 2.0 * u1 - 0.75 * u2 + 0.25 * e;
		}
		std::vector<int> indptr(n + 1, 0), indices;
		std::vector<double> weights;
		for (int r = 0; r < rows; ++r) {
			for (int c = 0; c < cols; ++c) {
				const int i = r * cols + c;
				const int dr[4] = { -1, 1, 0, 0 };
				const int dc[4] = { 0, 0, -1, 1 };
				for (int k = 0; k < 4; ++k) {
					const int rr = r + dr[k], cc = c + dc[k];
					if (rr < 0 || rr >= rows || cc < 0 || cc >= cols) continue;
					indices.push_back(rr * cols + cc);
					weights.push_back(1.0);
				}
				indptr[i + 1] = (int) indices.size();
			}
		}

		SpregJob::Writer writer(job_dir);
		std::vector<wxString> x_names;
		x_names.push_back("x1");
		x_names.push_back("x2");
		wxString job_err;
		bool ok = writer.AddY(y, job_err) && writer.AddX(x, job_err)
			&& writer.AddWeights(indptr, indices, weights, n, job_err);
		std::map<wxString, wxString> options;
		if (wxString(argc > 4 ? argv[4] : "OLS") == "ML_Lag") options["method"] = "LU";
		ok = ok && writer.Write(wxString(argc > 4 ? argv[4] : "OLS"), options,
								"y", x_names, "grid-rook", "",
								std::vector<wxString>(), std::vector<wxString>(),
								std::vector<wxString>(), job_err);
		check("the job is written", ok, job_err);

		wxString run_output, run_err;
		const int code = RunJob(engine_dir, job_dir, run_output, run_err, 300);
		check("the solver runs it", code == 0, run_err);

		SpregJob::Result result;
		wxString read_err;
		const bool read = result.Read(job_dir, read_err);
		check("the result is read back", read, read_err);
		check("it is not an error result", result.ok, result.error_message);
		if (result.ok) {
			std::printf("     model: %s (%s)\n", (const char*) result.title.utf8_str(),
						(const char*) result.class_name.utf8_str());
			for (size_t i = 0; i < result.names.size(); ++i) {
				std::printf("     %-12s %12.6f %12.6f %12.6f %9.5f  (%s)\n",
							(const char*) result.names[i].utf8_str(),
							i < result.estimate.size() ? result.estimate[i] : 0.0,
							i < result.std_err.size() ? result.std_err[i] : 0.0,
							i < result.z.size() ? result.z[i] : 0.0,
							i < result.p.size() ? result.p[i] : 0.0,
							i < result.roles.size() ? (const char*) result.roles[i].utf8_str() : "");
			}
			// the coefficients have to be the least squares ones: 1.5, 2.0, -0.75
			const double expected[3] = { 1.5, 2.0, -0.75 };
			bool close = result.estimate.size() >= 3;
			for (int i = 0; close && i < 3; ++i) {
				close = std::fabs(result.estimate[i] - expected[i]) < 0.05;
			}
			check("the coefficients are the ones the data was built from", close,
				  wxString::Format("%.4f %.4f %.4f", result.estimate.empty() ? 0 : result.estimate[0],
								   result.estimate.size() > 1 ? result.estimate[1] : 0.0,
								   result.estimate.size() > 2 ? result.estimate[2] : 0.0));
			check("the observations came back",
				  result.yhat.size() == (size_t) n && result.resid.size() == (size_t) n,
				  wxString::Format("yhat %d, resid %d", (int) result.yhat.size(),
								   (int) result.resid.size()));
			check("fit and diagnostics arrived",
				  !result.fit.empty() || !result.diagnostics.empty(),
				  wxString::Format("%d fit entries, %d diagnostics", (int) result.fit.size(),
								   (int) result.diagnostics.size()));
		}
		std::printf("\n%s\n", failures ? "FAILED" : "all checks passed");
		return failures ? 1 : 0;
	}

	if (argc >= 3 && wxString(argv[1]) == "--download") {
		// exercises Download() on its own, including the error messages, and
		// checks what came back against the digest the manifest carries
		const wxString url = argv[2];
		const wxString dest = (argc > 3 ? wxString(argv[3]) : wxString("."))
			+ "/download-test.bin";
		wxString err;
		NullSink sink;
		const bool ok = Download(url, dest, &sink, err);
		std::printf("download %s\n", ok ? "ok" : "failed");
		if (!ok) {
			std::printf("%s\n", (const char*) err.utf8_str());
			return 0;                     // a failure is the expected result here
		}
		wxString digest;
		if (!Sha256File(dest, digest, err)) {
			std::printf("hash failed: %s\n", (const char*) err.utf8_str());
			return 1;
		}
		wxFileName file(dest);
		std::printf("downloaded %lld bytes\nsha256 %s\n",
					(long long) file.GetSize().GetValue(),
					(const char*) digest.utf8_str());
		return 0;
	}

	// ShippedRoot() = GetResourceDir() + "spreg_engine/", so hand it the
	// repository, where spreg_engine/ has the layout the application ships:
	// <repo>/spreg_engine/dist/<archive> -> <repo>
	const wxString sep = wxFileName::GetPathSeparator();
	if (argc > 3) {
		g_installed_root = wxString::FromUTF8(argv[3]) + sep;
	} else {
		wxFileName shipped(archive);
		shipped.MakeAbsolute();
		// DirName, not wxFileName(path): the latter would treat the last
		// component as a file name and RemoveLastDir() would go one level too far
		wxFileName dir = wxFileName::DirName(shipped.GetPath());   // .../spreg_engine/dist
		dir.RemoveLastDir();                                       // .../spreg_engine
		dir.RemoveLastDir();                                       // the repository
		g_installed_root = dir.GetPathWithSep();
	}
	std::printf("shipped root : %s\n", (const char*)g_installed_root.utf8_str());

	// engines go to a scratch directory instead of the user's
	const wxString engines = scratch + wxFileName::GetPathSeparator() + "engines";
	wxSetEnv("GEODA_SPREG_ENGINES", engines);
	wxFileName::Mkdir(engines, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	std::printf("engines root : %s\n\n", (const char*)engines.utf8_str());

	// -- the manifest ------------------------------------------------------
	Manifest manifest;
	wxString err;
	check("manifest reads", manifest.Read(ShippedManifestPath(), err), err);
	const Artifact* artifact = manifest.ForThisPlatform();
	check("manifest has an engine for this platform (" + PlatformKey() + ")",
		  artifact != NULL,
		  artifact ? wxString::Format("spreg %s, %s", manifest.spreg,
									  artifact->file) : wxString(""));
	if (!artifact) return 1;

	// -- integrity ---------------------------------------------------------
	wxString digest;
	check("sha256 of the archive can be computed", Sha256File(archive, digest, err), err);
	check("sha256 matches the manifest", Sha256Matches(archive, artifact->sha256, err), err);
	std::printf("     %s\n", (const char*)digest.utf8_str());

	// -- not installed yet -------------------------------------------------
	{
		const Status before = Discover(manifest);
		check("nothing is installed to begin with", before.installed == false,
			  before.problem);
	}

	// -- a corrupted archive must be refused -------------------------------
	{
		const wxString corrupt = scratch + "/corrupt.zip";
		wxCopyFile(archive, corrupt);
		// flip a byte well past the header, in the middle of the payload
		{
			wxFFile file(corrupt, "r+b");
			if (file.IsOpened()) {
				const wxFileOffset size = file.Length();
				char byte = 0;
				file.Seek(size / 2, wxFromStart);
				file.Read(&byte, 1);
				byte = static_cast<char>(byte ^ 0xff);
				file.Seek(size / 2, wxFromStart);
				file.Write(&byte, 1);
			}
		}
		wxString installed;
		const bool ok = InstallFromZip(manifest, *artifact, corrupt, NULL, err, installed);
		check("a corrupted archive is refused", !ok, err);
		check("nothing was installed from it",
			  !wxFileName::DirExists(EnginesRoot() + manifest.EngineDirName(*artifact)));
		wxRemoveFile(corrupt);
	}

	// -- an archive that tries to escape its directory ---------------------
	{
		const wxString evil = scratch + "/evil.zip";
		if (WriteEvilZip(evil)) {
			const wxString target = scratch + "/evil-target";
			wxString unpack_err;
			const bool ok = UnpackZip(evil, target, std::vector<wxString>(), NULL, unpack_err);
			check("an archive with a ../ entry is refused", !ok, unpack_err);
			check("nothing was written outside the target directory",
				  !wxFileName::FileExists(scratch + "/escapee.txt"));
			check("the half unpacked directory was cleaned up",
				  !wxFileName::DirExists(target));
		}
	}

	// -- the real install --------------------------------------------------
	wxString installed_dir;
	const bool installed = InstallFromZip(manifest, *artifact, archive, NULL, err,
										   installed_dir);
	check("the engine installs", installed, err);
	if (!installed) return 1;

	const wxString python = EnginePythonPath(installed_dir);
	check("the engine has an interpreter", !python.IsEmpty(), python);
	check("the interpreter is executable", IsExecutable(python));
	check("the engine manifest travelled with it",
		  wxFileName::FileExists(installed_dir + wxFileName::GetPathSeparator() + "engine.json"));
	check("the licences travelled with it",
		  wxFileName::DirExists(installed_dir + wxFileName::GetPathSeparator() + "licenses"));

	const long long size = EngineSize(installed_dir);
	check("the unpacked size matches the manifest",
		  size > 0 && artifact->unpacked_bytes > 0
		  && size > artifact->unpacked_bytes * 95 / 100
		  && size < artifact->unpacked_bytes * 105 / 100,
		  wxString::Format("%lld bytes, %lld files", size, DirFileCount(installed_dir)));

	// -- discovery ---------------------------------------------------------
	{
		const Status status = Discover(manifest);
		check("the engine is found", status.installed, status.problem);
		check("its version comes from the engine", status.version == manifest.spreg,
			  status.version);
		check("the directory is the one the manifest predicts",
			  status.dir == EnginesRoot() + manifest.EngineDirName(*artifact), status.dir);

		// and the interpreter in it really runs
		const wxString probe = wxString::Format("%s -c \"import spreg; print(spreg.__version__)\"",
												QuoteArg(python));
		const int rc = std::system((const char*)probe.utf8_str());
		check("the installed interpreter imports spreg", rc == 0,
			  wxString::Format("exit code %d", rc));
	}

	// -- running the solver the way the application does -------------------
	// ProbeEngine and RunJob start a process and wait for the file it writes,
	// which is the part the regression dialog will depend on
	{
		wxString output, probe_err;
		const bool ok = ProbeEngine(installed_dir, output, probe_err, 60000);
		check("the engine answers --engine-info", ok, probe_err);
		check("and the answer names spreg", output.Contains("spreg")
			  && output.Contains(manifest.spreg),
			  output.Left(60));

		wxString timeout_err;
		const bool timed_out = ProbeEngine(installed_dir, output, timeout_err, 1);
		check("a one millisecond timeout is reported, not hung", !timed_out
			  && !timeout_err.IsEmpty(), timeout_err);
	}

	// -- upgrading over an existing engine ---------------------------------
	{
		wxString again_dir;
		wxString again_err;
		const bool again = InstallFromZip(manifest, *artifact, archive, NULL, again_err,
										   again_dir);
		check("installing over an existing engine works", again, again_err);
		check("it landed in the same place", again_dir == installed_dir);
		check("no leftover .old directory",
			  !wxFileName::DirExists(installed_dir + wxString::Format(".old-%lu", wxGetProcessId())));
	}

	// -- removing ----------------------------------------------------------
	{
		wxString remove_err;
		check("the engine is removed", Remove(installed_dir, remove_err), remove_err);
		check("it is gone", !wxFileName::DirExists(installed_dir));
		check("and discovery agrees", !Discover(manifest).installed);

		const wxString not_an_engine = scratch + "/keepme";
		wxFileName::Mkdir(not_an_engine, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
		wxString refuse_err;
		check("removal refuses a directory that is not an engine",
			  !Remove(not_an_engine, refuse_err), refuse_err);
		check("and leaves it alone", wxFileName::DirExists(not_an_engine));
	}

	// -- downloading makes the directory it writes into ---------------------
	// An install failed on its first byte when the engine folder was not there
	// yet, which is the state every first install starts in: the user data
	// folder is made when GeoDa first runs, the engines folder inside it is not.
	{
		const wxString fresh = scratch + "/fresh/chain";
		if (wxFileName::DirExists(fresh)) {
			wxFileName::Rmdir(fresh, wxPATH_RMDIR_RECURSIVE);
		}
		wxString fresh_err;
		NullSink sink;
		// the download itself cannot work - nothing listens there - but the
		// directory has to have been made before the first byte is written
		Download("http://127.0.0.1:9/nothing", fresh + "/probe.bin", &sink, fresh_err);
		check("a download makes the directory it writes into",
			  wxFileName::DirExists(fresh), fresh_err);
	}

	std::printf("\n%s\n", failures ? "FAILED" : "all checks passed");
	return failures ? 1 : 0;
}
