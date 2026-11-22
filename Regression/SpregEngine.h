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

#ifndef __GEODA_CENTER_SPREG_ENGINE_H__
#define __GEODA_CENTER_SPREG_ENGINE_H__

#include <map>
#include <vector>
#include <wx/string.h>

/**
 * The Python regression engine that backs the advanced models in the
 * Regression dialog (regimes, spatial Durbin/SLX, GMM and IV, probit,
 * specification search, regression-based regionalization).
 *
 * The engine is a self-contained CPython plus a pinned wheel set, built once
 * per platform by spreg_engine/tools/build_engine.py and published as a zip.
 * The application ships the *solver* (spreg_engine/solver/, in the resource
 * directory) and the manifest, downloads the engine on demand into the user's
 * data directory, verifies its sha256 and unpacks it there.  Nothing here talks
 * to spreg directly: the solver does, through the job protocol described in
 * spreg_engine/PROTOCOL.md.
 *
 * See spreg_engine/README.md for the install flow this implements.
 */
namespace SpregEngine {

/** One platform's engine archive, as described in engines.json. */
struct Artifact {
	wxString key;            // "macos-arm64"
	wxString file;           // archive file name
	wxString url;
	wxString sha256;
	wxString python;         // version of the interpreter inside
	long long size_bytes;    // compressed, 0 when not built yet
	long long unpacked_bytes;
	std::vector<wxString> executables;  // paths to chmod +x after unpacking
	bool built;              // false: the artifact does not exist yet

	Artifact() : size_bytes(0), unpacked_bytes(0), built(false) {}
};

struct Manifest {
	int protocol;
	int manifest;
	wxString spreg;
	wxString python_series;
	wxString solver;
	wxString engine_dir_template;   // "engines/spreg-{spreg}-py{py}-{platform}"
	wxString unpack_root;           // "python"
	std::map<wxString, Artifact> artifacts;

	Manifest() : protocol(0), manifest(0) {}

	/** Reads engines.json. Returns false and fills err on any problem. */
	bool Read(const wxString& path, wxString& err);

	/** The entry for the platform GeoDa is running on, or null. */
	const Artifact* ForThisPlatform() const;

	/** The directory name of the engine, i.e. "spreg-1.9.1-py313-macos-arm64". */
	wxString EngineDirName(const Artifact& artifact) const;
};

/** Where the manifest and the solver live, i.e. <resources>/spreg_engine/. */
wxString ShippedRoot();
wxString ShippedManifestPath();
wxString SolverScriptPath();

/** Where downloaded engines live: <user data>/GeoDa/engines/. */
wxString EnginesRoot();

/** "macos-arm64", "macos-x86_64", "windows-x86_64", "linux-x86_64", ... */
wxString PlatformKey();

/** An installed engine. */
struct Status {
	bool installed;
	wxString dir;        // engine directory, empty when not installed
	wxString python;     // path to the interpreter
	wxString version;    // spreg version reported by the engine
	wxString problem;    // why it is not usable, when installed == false

	Status() : installed(false) {}
};

/** Looks for an installed, usable engine; never downloads anything. */
Status Discover(const Manifest& manifest);

/** The interpreter inside an engine directory, empty if there is none. */
wxString EnginePythonPath(const wxString& engine_dir);

/** Bytes on disk of an installed engine, or -1 if it cannot be measured. */
long long EngineSize(const wxString& engine_dir);

// ---------------------------------------------------------------------------
// integrity
// ---------------------------------------------------------------------------

/** Lower-case hex sha256 of a file. */
bool Sha256File(const wxString& path, wxString& hex_out, wxString& err);
/** Convenience: does the file match the expected digest? */
bool Sha256Matches(const wxString& path, const wxString& expected, wxString& err);

// ---------------------------------------------------------------------------
// installing
// ---------------------------------------------------------------------------

/** Reports progress and cancellation to whatever UI is driving an install. */
class ProgressSink {
public:
	virtual ~ProgressSink() {}
	/** done/total in bytes (total < 0 when unknown); return false to cancel. */
	virtual bool OnProgress(long long done, long long total,
							const wxString& phase) = 0;
	/** True when the user asked to stop. */
	virtual bool Cancelled() = 0;
};

/** A sink that does nothing, for batch use. */
class NullSink : public ProgressSink {
public:
	virtual bool OnProgress(long long, long long, const wxString&) { return true; }
	virtual bool Cancelled() { return false; }
};

/** Downloads a url to a file. Follows redirects; reports through sink. */
bool Download(const wxString& url, const wxString& dest, ProgressSink* sink,
			  wxString& err);

/**
 * Unpacks one of our engine archives into dest_dir (which is created), and
 * restores the execute bits of the listed paths: wx does not do that itself,
 * and without it the interpreter inside the engine cannot be run.
 */
bool UnpackZip(const wxString& zip_path, const wxString& dest_dir,
			   const std::vector<wxString>& executables, ProgressSink* sink,
			   wxString& err);

/**
 * Installs an already downloaded archive: verifies its sha256 against the
 * manifest, unpacks it next to its final location and renames it into place, so
 * that an interrupted install never leaves a half-unpacked engine behind.
 */
bool InstallFromZip(const Manifest& manifest, const Artifact& artifact,
					const wxString& zip_path, ProgressSink* sink,
					wxString& err, wxString& installed_dir);

/** Download, verify, install. Equivalent to Download() + InstallFromZip(). */
bool InstallFromUrl(const Manifest& manifest, const Artifact& artifact,
					ProgressSink* sink, wxString& err, wxString& installed_dir);

/** Deletes an installed engine directory. */
bool Remove(const wxString& engine_dir, wxString& err);

// ---------------------------------------------------------------------------
// running
// ---------------------------------------------------------------------------

/**
 * Runs the solver with extra arguments and returns what it wrote to --out.
 * ProbeEngine is one case of it; SpregJob::ListModels is another.
 */
bool RunSolverCommand(const wxString& engine_dir, const wxString& args,
					  wxString& output, wxString& err, int timeout_sec = 120);

/**
 * Runs the solver and returns what it printed. Used to check that an engine
 * works right after installing it; the regression dialog uses RunJob() below
 * in the same way.
 */
bool ProbeEngine(const wxString& engine_dir, wxString& output, wxString& err,
				 int timeout_ms = 120000);

/**
 * Runs one job through the solver. job_dir must already contain job.json and
 * data.bin (see PROTOCOL.md); on return, result.json and friends are in it.
 * Returns the process exit code, or -1 if it could not be started or timed out.
 */
int RunJob(const wxString& engine_dir, const wxString& job_dir, wxString& output,
		   wxString& err, int timeout_sec = 900, ProgressSink* sink = 0);

/** Quotes one argument for the shell, portably. */
wxString QuoteArg(const wxString& arg);

} // namespace SpregEngine

#endif
