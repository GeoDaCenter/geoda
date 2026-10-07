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

#include <cstdio>
#include <string>
#include <vector>

#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/zipstrm.h>
#include <wx/wfstream.h>
#include <wx/dir.h>

#include "../../Regression/SpregEngine.h"
#include "../../GenUtils.h"

using namespace SpregEngine;

// ---------------------------------------------------------------------------
// stubs for the two path helpers SpregEngine.cpp uses.  The resource directory
// points at the repository, whose spreg_engine/ directory is laid out exactly
// like the one inside the installed application.
// ---------------------------------------------------------------------------

static wxString g_installed_root;      // the equivalent of Contents/Resources/

wxString GenUtils::GetResourceDir() { return g_installed_root; }
wxString GenUtils::GetExeDir() { return g_installed_root; }

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
		return 2;
	}
	const wxString archive = wxString::FromUTF8(argv[1]);
	const wxString scratch = wxString::FromUTF8(argv[2]);

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

	std::printf("\n%s\n", failures ? "FAILED" : "all checks passed");
	return failures ? 1 : 0;
}
