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

#include "SpregEngine.h"
#include "SpregSha256.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/filefn.h>
#include <wx/stream.h>
#include <wx/tokenzr.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/stdpaths.h>
#include <wx/process.h>
#include <wx/utils.h>

#include <curl/curl.h>
#include <json_spirit/json_spirit.h>

#include "../GenUtils.h"

#ifndef __WXMSW__
#include <sys/stat.h>
#include <sys/wait.h>
#include <cerrno>
#endif

namespace {

const size_t kCopyBuffer = 64 * 1024;

wxString Sep() { return wxFileName::GetPathSeparator(); }

#ifdef __WXMSW__
wxString PythonExeName() { return "python.exe"; }
#else
wxString PythonExeName() { return "bin/python3"; }
#endif

/** The writable per-user directory GeoDa already uses for its own files. */
wxString UserBaseDir(bool create)
{
	wxString base;
#ifdef __WXMAC__
	// ~/Library/Application Support/GeoDa
	base = wxStandardPaths::Get().GetUserDataDir();
#elif defined(__WXMSW__)
	// AppData\Roaming\GeoDa, as elsewhere in GeoDa
	base = wxStandardPaths::Get().GetUserConfigDir() + Sep() + "GeoDa";
#else
	// ~/.geoda, as elsewhere in GeoDa
	base = wxStandardPaths::Get().GetUserConfigDir() + Sep() + ".geoda";
#endif
	if (create && !wxDirExists(base)) {
		wxFileName::Mkdir(base, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	}
	return base;
}

bool ReadWholeFile(const wxString& path, std::string& out, wxString& err)
{
	std::ifstream in(path.fn_str(), std::ios::binary);
	if (!in) {
		err = wxString::Format(_("Could not open %s"), path);
		return false;
	}
	out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
	return true;
}

// ---------------------------------------------------------------------------
// json_spirit helpers (the manifest and the solver's answers are both JSON)
// ---------------------------------------------------------------------------

const json_spirit::Value* FindMember(const json_spirit::Object& obj,
									 const char* name)
{
	for (json_spirit::Object::const_iterator it = obj.begin(); it != obj.end(); ++it) {
		if (it->name_ == name) return &it->value_;
	}
	return NULL;
}

wxString AsString(const json_spirit::Value* value)
{
	if (!value || value->type() != json_spirit::str_type) return wxEmptyString;
	return wxString::FromUTF8(value->get_str().c_str());
}

bool AsBool(const json_spirit::Value* value, bool fallback)
{
	if (!value || value->type() != json_spirit::bool_type) return fallback;
	return value->get_bool();
}

long long AsInt(const json_spirit::Value* value, long long fallback)
{
	if (!value) return fallback;
	if (value->type() == json_spirit::int_type) {
		return static_cast<long long>(value->get_int64());
	}
	if (value->is_uint64()) return static_cast<long long>(value->get_uint64());
	if (value->type() == json_spirit::real_type) {
		return static_cast<long long>(value->get_real());
	}
	return fallback;
}

// ---------------------------------------------------------------------------
// process helpers
// ---------------------------------------------------------------------------

/**
 * Has the child finished?  wxProcess::Exists() is not enough on unix: a child
 * that has exited but has not been reaped is a zombie, and still exists, so an
 * engine that fails on start would look like one that hangs.  waitpid() answers
 * the question and reaps it.  On Windows a process that ends stops existing.
 */
bool ChildFinished(int pid, int* exit_code)
{
#ifdef __WXMSW__
	if (wxProcess::Exists(pid)) return false;
	*exit_code = -1;
	return true;
#else
	int status = 0;
	const pid_t got = waitpid(static_cast<pid_t>(pid), &status, WNOHANG);
	if (got == static_cast<pid_t>(pid)) {
		*exit_code = WIFEXITED(status) ? WEXITSTATUS(status)
									   : -WTERMSIG(status);
		return true;
	}
	if (got < 0 && errno == ECHILD) {
		*exit_code = -1;        // wx's handler got there first
		return true;
	}
	return false;
#endif
}

struct ProcessResult {
	bool start_failed;
	bool finished;        // the solver wrote its answer, or the process went away
	bool timed_out;
	bool cancelled;
	int exit_code;        // -1 when it was killed or reaped elsewhere
	wxString output;

	ProcessResult() : start_failed(false), finished(false), timed_out(false),
		cancelled(false), exit_code(-1) {}
};

/**
 * Runs the solver and waits for the file it is expected to produce.
 *
 * Deliberately does not use wxProcess::OnTerminate(): that is delivered through
 * the event loop, which is not running while a dialog is waiting, so the exit
 * code would never arrive.  Instead the loop polls for the solver's own output
 * file - result.json or the probe's answer - reads whatever the child prints as
 * it appears (so a chatty child cannot fill the pipe), and kills the process on
 * cancel or timeout.
 */
ProcessResult RunProcess(const wxString& cmd, const wxString& wait_for_file,
						 int timeout_sec, SpregEngine::ProgressSink* sink)
{
	ProcessResult result;
	wxLogMessage("SpregEngine: running %s", cmd);

	wxProcess* process = new wxProcess(wxPROCESS_REDIRECT);
	const long pid = wxExecute(cmd, wxEXEC_ASYNC, process);
	if (pid == 0) {
		delete process;
		result.start_failed = true;
		return result;
	}

	wxStopWatch watch;
	const long limit_ms = static_cast<long>(timeout_sec) * 1000;
	char buffer[4096];

	for (;;) {
		wxInputStream* out = process->GetInputStream();
		wxInputStream* err = process->GetErrorStream();
		while (out && out->CanRead()) {
			out->Read(buffer, sizeof(buffer));
			const size_t n = out->LastRead();
			if (n == 0) break;
			result.output += wxString::FromUTF8(buffer, n);
		}
		while (err && err->CanRead()) {
			err->Read(buffer, sizeof(buffer));
			const size_t n = err->LastRead();
			if (n == 0) break;
			result.output += wxString::FromUTF8(buffer, n);
		}

		if (!wait_for_file.IsEmpty() && wxFileName::FileExists(wait_for_file)) {
			result.finished = true;
			break;
		}
		int exit_code = -1;
		if (ChildFinished(static_cast<int>(pid), &exit_code)) {
			// it stopped on its own; whatever it had to say is in the file it
			// was asked to write, or in the log next to the job
			result.finished = true;
			result.exit_code = exit_code;
			break;
		}

		if (sink && sink->Cancelled()) {
			result.cancelled = true;
		} else if (limit_ms > 0 && watch.Time() > limit_ms) {
			result.timed_out = true;
		}
		if (result.cancelled || result.timed_out) {
			wxLogMessage(result.cancelled ? "SpregEngine: cancelled"
										  : "SpregEngine: timed out");
			wxProcess::Kill(static_cast<int>(pid), wxSIGTERM, wxKILL_CHILDREN);
			for (int i = 0; i < 20 && wxProcess::Exists(static_cast<int>(pid)); ++i) {
				wxMilliSleep(100);
			}
			if (wxProcess::Exists(static_cast<int>(pid))) {
				wxProcess::Kill(static_cast<int>(pid), wxSIGKILL, wxKILL_CHILDREN);
			}
			break;
		}
		wxMilliSleep(50);
	}

	delete process;
	return result;
}

// ---------------------------------------------------------------------------
// curl
// ---------------------------------------------------------------------------

size_t WriteToFile(void* ptr, size_t size, size_t nmemb, void* stream)
{
	FILE* file = static_cast<FILE*>(stream);
	if (!file) return 0;
	return fwrite(ptr, size, nmemb, file);
}

struct TransferState {
	SpregEngine::ProgressSink* sink;
	long long reported;
	wxStopWatch watch;

	TransferState() : sink(NULL), reported(0) {}
};

int OnTransferProgress(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
					   curl_off_t, curl_off_t)
{
	TransferState* state = static_cast<TransferState*>(clientp);
	if (!state || !state->sink) return 0;
	const long long now = static_cast<long long>(dlnow);
	const long long total = static_cast<long long>(dltotal);
	// report at most every 200 ms: the sink redraws a progress dialog
	if (now != total && state->watch.Time() < 200 && now != state->reported) return 0;
	state->watch.Start();
	state->reported = now;
	const bool keep_going = state->sink->OnProgress(now, total, _("Downloading"));
	return keep_going ? 0 : 1;   // non-zero aborts the transfer
}

const char* kUserAgent = "GeoDa-spreg-engine-installer";

} // namespace

// ---------------------------------------------------------------------------
// manifest
// ---------------------------------------------------------------------------

namespace SpregEngine {

bool Manifest::Read(const wxString& path, wxString& err)
{
	artifacts.clear();

	std::string text;
	if (!ReadWholeFile(path, text, err)) return false;

	json_spirit::Value value;
	if (!json_spirit::read(text, value) || value.type() != json_spirit::obj_type) {
		err = wxString::Format(_("%s is not a valid engine manifest"), path);
		return false;
	}
	const json_spirit::Object& root = value.get_obj();

	protocol = static_cast<int>(AsInt(FindMember(root, "protocol"), 0));
	manifest = static_cast<int>(AsInt(FindMember(root, "manifest"), 0));
	spreg = AsString(FindMember(root, "spreg"));
	python_series = AsString(FindMember(root, "python_series"));
	solver = AsString(FindMember(root, "solver"));
	engine_dir_template = AsString(FindMember(root, "engine_dir_template"));
	unpack_root = AsString(FindMember(root, "unpack_root"));

	const json_spirit::Value* list = FindMember(root, "artifacts");
	if (!list || list->type() != json_spirit::obj_type) {
		err = wxString::Format(_("%s has no artifacts section"), path);
		return false;
	}
	const json_spirit::Object& entries = list->get_obj();
	for (json_spirit::Object::const_iterator it = entries.begin(); it != entries.end(); ++it) {
		if (it->value_.type() != json_spirit::obj_type) continue;
		const json_spirit::Object& node = it->value_.get_obj();

		Artifact artifact;
		artifact.key = wxString::FromUTF8(it->name_.c_str());
		artifact.file = AsString(FindMember(node, "file"));
		artifact.url = AsString(FindMember(node, "url"));
		artifact.sha256 = AsString(FindMember(node, "sha256"));
		artifact.python = AsString(FindMember(node, "python"));
		artifact.size_bytes = AsInt(FindMember(node, "size_bytes"), 0);
		artifact.unpacked_bytes = AsInt(FindMember(node, "unpacked_bytes"), 0);
		artifact.built = (AsString(FindMember(node, "status")) == "built")
			&& !artifact.sha256.IsEmpty() && artifact.size_bytes > 0;

		const json_spirit::Value* execs = FindMember(node, "executables");
		if (execs && execs->type() == json_spirit::array_type) {
			const json_spirit::Array& array = execs->get_array();
			for (size_t i = 0; i < array.size(); ++i) {
				if (array[i].type() == json_spirit::str_type) {
					artifact.executables.push_back(
						wxString::FromUTF8(array[i].get_str().c_str()));
				}
			}
		}
		artifacts[artifact.key] = artifact;
	}

	if (artifacts.empty()) {
		err = wxString::Format(_("%s lists no engine for any platform"), path);
		return false;
	}
	return true;
}

const Artifact* Manifest::ForThisPlatform() const
{
	const wxString key = PlatformKey();
	std::map<wxString, Artifact>::const_iterator it = artifacts.find(key);
	if (it == artifacts.end() || !it->second.built) return NULL;
	return &it->second;
}

wxString Manifest::EngineDirName(const Artifact& artifact) const
{
	wxString name = engine_dir_template.IsEmpty()
		? wxString("engines/spreg-{spreg}-py{py}-{platform}")
		: engine_dir_template;

	wxString py = python_series;
	py.Replace(".", "");                      // "3.13" -> "313"
	if (py.IsEmpty() && !artifact.python.IsEmpty()) {
		py = artifact.python.BeforeLast('.');
		py.Replace(".", "");
	}
	name.Replace("{spreg}", spreg);
	name.Replace("{py}", py);
	name.Replace("{platform}", artifact.key);
	// the template starts with the engines root, which EnginesRoot() provides,
	// so only its last component is wanted here
	return wxFileName(name).GetFullName();
}

// ---------------------------------------------------------------------------
// locations
// ---------------------------------------------------------------------------

wxString ShippedRoot()
{
#ifdef __WXMAC__
	return GenUtils::GetResourceDir() + "spreg_engine" + Sep();
#else
	return GenUtils::GetExeDir() + "spreg_engine" + Sep();
#endif
}

wxString ShippedManifestPath()
{
	return ShippedRoot() + "manifest" + Sep() + "engines.json";
}

wxString SolverScriptPath()
{
	return ShippedRoot() + "solver" + Sep() + "solve.py";
}

wxString EnginesRoot()
{
	// An administrator who prepares machines, or installs for several users at
	// once, can point GeoDa at an engine directory of their own; the same
	// variable makes the install path testable without a GUI.
	wxString override_dir;
	if (wxGetEnv("GEODA_SPREG_ENGINES", &override_dir) && !override_dir.IsEmpty()) {
		if (!wxDirExists(override_dir)) {
			wxFileName::Mkdir(override_dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
		}
		return override_dir + Sep();
	}
	return UserBaseDir(true) + Sep() + "engines" + Sep();
}

wxString PlatformKey()
{
#if defined(__WXMAC__)
	#if defined(__arm64__) || defined(__aarch64__)
		return "macos-arm64";
	#else
		return "macos-x86_64";
	#endif
#elif defined(__WXMSW__)
	#if defined(_M_ARM64) || defined(__aarch64__)
		return "windows-arm64";
	#else
		return "windows-x86_64";
	#endif
#else
	#if defined(__aarch64__) || defined(__arm__)
		return "linux-aarch64";
	#else
		return "linux-x86_64";
	#endif
#endif
}

wxString EnginePythonPath(const wxString& engine_dir)
{
	if (engine_dir.IsEmpty()) return wxEmptyString;
	const wxString path = engine_dir + Sep() + "python" + Sep() + PythonExeName();
	if (wxFileName::FileExists(path)) return path;
	return wxEmptyString;
}

long long EngineSize(const wxString& engine_dir)
{
	if (engine_dir.IsEmpty()) return -1;
	long long total = 0;
	wxDir dir(engine_dir);
	if (!dir.IsOpened()) return -1;
	wxString name;
	bool more = dir.GetFirst(&name, wxEmptyString, wxDIR_FILES | wxDIR_DIRS | wxDIR_HIDDEN);
	while (more) {
		const wxString path = engine_dir + Sep() + name;
		if (wxFileName::DirExists(path)) {
			const long long sub = EngineSize(path);
			if (sub < 0) return -1;
			total += sub;
		} else {
			wxFileName file(path);
			total += static_cast<long long>(file.GetSize().GetValue());
		}
		more = dir.GetNext(&name);
	}
	return total;
}

Status Discover(const Manifest& manifest)
{
	Status status;

	std::vector<wxString> candidates;
	const Artifact* artifact = manifest.ForThisPlatform();
	if (artifact) {
		candidates.push_back(EnginesRoot() + manifest.EngineDirName(*artifact));
	}

	// any other engine that is lying around (an older GeoDa's, or one installed
	// by hand): the newest one whose protocol matches will do
	{
		wxDir dir(EnginesRoot());
		wxString name;
		bool more = dir.IsOpened()
			&& dir.GetFirst(&name, "spreg-*", wxDIR_DIRS | wxDIR_HIDDEN);
		while (more) {
			candidates.push_back(EnginesRoot() + name);
			more = dir.GetNext(&name);
		}
	}
	std::sort(candidates.begin(), candidates.end());
	std::reverse(candidates.begin(), candidates.end());

	for (size_t i = 0; i < candidates.size(); ++i) {
		const wxString& dir = candidates[i];
		const wxString info_path = dir + Sep() + "engine.json";
		if (!wxFileName::FileExists(info_path)) continue;

		std::string text;
		wxString err;
		if (!ReadWholeFile(info_path, text, err)) continue;
		json_spirit::Value value;
		if (!json_spirit::read(text, value) || value.type() != json_spirit::obj_type) continue;
		const json_spirit::Object& root = value.get_obj();

		const int protocol = static_cast<int>(AsInt(FindMember(root, "protocol"), 0));
		if (protocol != manifest.protocol) {
			if (status.problem.IsEmpty()) {
				status.problem = wxString::Format(
					_("The installed engine at %s speaks protocol %d, this GeoDa %d."),
					dir, protocol, manifest.protocol);
			}
			continue;
		}
		const wxString python = EnginePythonPath(dir);
		if (python.IsEmpty()) {
			status.problem = wxString::Format(
				_("The engine at %s has no interpreter; install it again."), dir);
			continue;
		}
		status.installed = true;
		status.dir = dir;
		status.python = python;
		status.version = AsString(FindMember(root, "spreg"));
		status.problem = wxEmptyString;
		return status;
	}
	return status;
}

// ---------------------------------------------------------------------------
// integrity
// ---------------------------------------------------------------------------

bool Sha256File(const wxString& path, wxString& hex_out, wxString& err)
{
	std::ifstream in(path.fn_str(), std::ios::binary);
	if (!in) {
		err = wxString::Format(_("Could not read %s"), path);
		return false;
	}
	Sha256 hash;
	std::vector<char> buffer(kCopyBuffer);
	while (in) {
		in.read(&buffer[0], static_cast<std::streamsize>(buffer.size()));
		const std::streamsize got = in.gcount();
		if (got > 0) hash.Update(&buffer[0], static_cast<size_t>(got));
	}
	hex_out = wxString::FromUTF8(hash.HexDigest().c_str());
	return true;
}

bool Sha256Matches(const wxString& path, const wxString& expected, wxString& err)
{
	wxString actual;
	if (!Sha256File(path, actual, err)) return false;
	if (actual.CmpNoCase(expected) == 0) return true;
	err = wxString::Format(_("%s\n\nThe file is %s but should be %s."),
						   path, actual, expected.Lower());
	return false;
}

// ---------------------------------------------------------------------------
// installing
// ---------------------------------------------------------------------------

bool Download(const wxString& url, const wxString& dest, ProgressSink* sink,
			  wxString& err)
{
	CURL* curl = curl_easy_init();
	if (!curl) {
		err = _("libcurl could not be initialized.");
		return false;
	}

	FILE* file = fopen(dest.fn_str(), "wb");
	if (!file) {
		curl_easy_cleanup(curl);
		err = wxString::Format(_("Could not write to %s"), dest);
		return false;
	}

	TransferState state;
	state.sink = sink;
	state.watch.Start();

	char error_buffer[CURL_ERROR_SIZE];
	error_buffer[0] = '\0';

	const wxCharBuffer url_bytes = url.utf8_str();
	curl_easy_setopt(curl, CURLOPT_URL, url_bytes.data());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToFile);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);   // github release assets redirect
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, kUserAgent);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
	curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1024L);   // bail out on a dead link
	curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60L);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, OnTransferProgress);
	curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &state);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);

	const CURLcode code = curl_easy_perform(curl);
	long http_status = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status);
	curl_easy_cleanup(curl);
	fclose(file);

	if (code == CURLE_OK && http_status >= 200 && http_status < 300) return true;

	wxRemoveFile(dest);
	if (sink && sink->Cancelled()) {
		err = _("The download was cancelled.");
	} else if (code == CURLE_ABORTED_BY_CALLBACK) {
		err = _("The download was cancelled.");
	} else if (code != CURLE_OK) {
		err = wxString::Format(_("Could not download the engine:\n%s"),
							   error_buffer[0] ? wxString::FromUTF8(error_buffer)
											   : wxString(curl_easy_strerror(code)));
	} else {
		err = wxString::Format(_("The server answered with HTTP status %ld."), http_status);
	}
	return false;
}

bool UnpackZip(const wxString& zip_path, const wxString& dest_dir,
			   const std::vector<wxString>& executables, ProgressSink* sink,
			   wxString& err)
{
	if (!wxFileName::DirExists(dest_dir)
		&& !wxFileName::Mkdir(dest_dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
		err = wxString::Format(_("Could not create %s"), dest_dir);
		return false;
	}

	wxFFileInputStream file_in(zip_path);
	if (!file_in.IsOk()) {
		err = wxString::Format(_("Could not open %s"), zip_path);
		return false;
	}
	wxZipInputStream zip(file_in);

	std::vector<wxString> extracted;
	wxZipEntry* entry;
	bool ok = true;
	while ((entry = zip.GetNextEntry()) != NULL) {
		const wxString name = entry->GetName();
		// never trust an archive: refuse absolute paths and any ".."
		const wxString parts = name;
		bool unsafe = name.StartsWith("/") || name.StartsWith("\\")
			|| name.Contains(":");
		wxStringTokenizer tokens(name, "/\\");
		while (tokens.HasMoreTokens()) {
			const wxString part = tokens.GetNextToken();
			if (part == "..") unsafe = true;
		}
		if (unsafe) {
			err = wxString::Format(
				_("The engine archive contains an unsafe path (%s) and was not unpacked."),
				name);
			delete entry;
			ok = false;
			break;
		}

		wxFileName target(dest_dir + Sep() + name);
		target.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_TILDE);

		if (entry->IsDir()) {
			if (!wxFileName::DirExists(target.GetFullPath())) {
				wxFileName::Mkdir(target.GetFullPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
			}
		} else {
			const wxString dir = target.GetPath();
			if (!dir.IsEmpty() && !wxFileName::DirExists(dir)) {
				wxFileName::Mkdir(dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
			}
			wxFileOutputStream out(target.GetFullPath());
			if (!out.IsOk()) {
				err = wxString::Format(_("Could not write %s"), target.GetFullPath());
				delete entry;
				ok = false;
				break;
			}
			zip.Read(out);
			out.Close();
			extracted.push_back(target.GetFullPath());
		}
		delete entry;

		if (sink && sink->OnProgress(0, -1, wxString::Format(_("Unpacking %s"), name))
			== false) {
			err = _("The installation was cancelled.");
			ok = false;
			break;
		}
	}

	if (!ok) {
		// leave nothing half unpacked
		wxFileName::Rmdir(dest_dir, wxPATH_RMDIR_RECURSIVE);
		return false;
	}
	if (extracted.empty()) {
		err = wxString::Format(_("%s contains no files."), zip_path);
		wxFileName::Rmdir(dest_dir, wxPATH_RMDIR_RECURSIVE);
		return false;
	}

#ifndef __WXMSW__
	// wx does not restore unix permissions, and without the execute bit the
	// interpreter inside the engine cannot be started at all
	for (size_t i = 0; i < executables.size(); ++i) {
		const wxString path = dest_dir + Sep() + executables[i];
		if (!wxFileName::FileExists(path)) continue;
		if (chmod(path.fn_str(), 0755) != 0) {
			err = wxString::Format(_("Could not make %s executable."), path);
			wxFileName::Rmdir(dest_dir, wxPATH_RMDIR_RECURSIVE);
			return false;
		}
	}
#endif

	return true;
}

bool InstallFromZip(const Manifest& manifest, const Artifact& artifact,
					const wxString& zip_path, ProgressSink* sink,
					wxString& err, wxString& installed_dir)
{
	installed_dir = wxEmptyString;

	if (!wxFileName::FileExists(zip_path)) {
		err = wxString::Format(_("%s does not exist."), zip_path);
		return false;
	}
	if (artifact.size_bytes > 0) {
		wxFileName archive(zip_path);
		const long long size = static_cast<long long>(archive.GetSize().GetValue());
		if (size != artifact.size_bytes) {
			err = wxString::Format(
				_("The engine archive is %lld bytes, but the manifest says %lld; "
				  "the download is incomplete."), size, artifact.size_bytes);
			return false;
		}
	}
	if (!artifact.sha256.IsEmpty()
		&& !Sha256Matches(zip_path, artifact.sha256, err)) {
		return false;
	}

	const wxString target = EnginesRoot() + manifest.EngineDirName(artifact);
	const wxString temp = wxString::Format("%s.tmp-%lu", target, wxGetProcessId());

	wxFileName::Rmdir(temp, wxPATH_RMDIR_RECURSIVE);
	if (!UnpackZip(zip_path, temp, artifact.executables, sink, err)) {
		return false;
	}

	// swap the new engine in: keep the old one until the new one is in place
	const wxString previous = wxString::Format("%s.old-%lu", target, wxGetProcessId());
	if (wxFileName::DirExists(target)) {
		wxFileName::Rmdir(previous, wxPATH_RMDIR_RECURSIVE);
		if (!wxRenameFile(target, previous, true)) {
			wxFileName::Rmdir(temp, wxPATH_RMDIR_RECURSIVE);
			err = wxString::Format(_("Could not replace the engine at %s"), target);
			return false;
		}
	}
	if (!wxRenameFile(temp, target, true)) {
		wxFileName::Rmdir(temp, wxPATH_RMDIR_RECURSIVE);
		if (wxFileName::DirExists(previous)) wxRenameFile(previous, target, true);
		err = wxString::Format(_("Could not move the engine into %s"), target);
		return false;
	}
	wxFileName::Rmdir(previous, wxPATH_RMDIR_RECURSIVE);

	installed_dir = target;
	return true;
}

bool InstallFromUrl(const Manifest& manifest, const Artifact& artifact,
					ProgressSink* sink, wxString& err, wxString& installed_dir)
{
	installed_dir = wxEmptyString;
	if (artifact.url.IsEmpty()) {
		err = _("The manifest has no download location for this platform.");
		return false;
	}

	const wxString archive = EnginesRoot() + artifact.file + ".part";
	wxRemoveFile(archive);

	if (!Download(artifact.url, archive, sink, err)) {
		wxRemoveFile(archive);
		return false;
	}

	if (sink && sink->OnProgress(0, -1, _("Checking the download"))) {
		// fall through: verification happens inside InstallFromZip
	}

	const bool ok = InstallFromZip(manifest, artifact, archive, sink, err, installed_dir);
	wxRemoveFile(archive);
	return ok;
}

bool Remove(const wxString& engine_dir, wxString& err)
{
	if (engine_dir.IsEmpty() || !wxFileName::DirExists(engine_dir)) {
		err = _("There is no engine to remove.");
		return false;
	}
	// only ever remove something that looks like one of ours
	if (!wxFileName(engine_dir).GetFullName().StartsWith("spreg-")) {
		err = wxString::Format(_("Refusing to remove %s: it is not an engine directory."),
							   engine_dir);
		return false;
	}
	if (!wxFileName::Rmdir(engine_dir, wxPATH_RMDIR_RECURSIVE)) {
		err = wxString::Format(_("Could not remove %s"), engine_dir);
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
// running
// ---------------------------------------------------------------------------

wxString QuoteArg(const wxString& arg)
{
	if (!arg.Contains(" ") && !arg.Contains("\t") && !arg.Contains("\"")) {
		return arg;
	}
	return "\"" + arg + "\"";
}

bool ProbeEngine(const wxString& engine_dir, wxString& output, wxString& err,
				 int timeout_ms)
{
	const wxString python = EnginePythonPath(engine_dir);
	if (python.IsEmpty()) {
		err = wxString::Format(_("There is no interpreter in %s."), engine_dir);
		return false;
	}
	const wxString solver = SolverScriptPath();
	if (!wxFileName::FileExists(solver)) {
		err = wxString::Format(
			_("The solver is missing from this installation of GeoDa (%s)."), solver);
		return false;
	}

	// the answer goes to a file: the app waits for the file, not for an exit
	// code, which is the only thing that works with a blocked event loop
	const wxString answer = wxFileName::CreateTempFileName("geoda-spreg-probe");
	wxRemoveFile(answer);
	const wxString cmd = QuoteArg(python) + " " + QuoteArg(solver)
		+ " --engine-info --out " + QuoteArg(answer);

	const ProcessResult result = RunProcess(cmd, answer,
											(timeout_ms + 999) / 1000, NULL);
	if (result.start_failed) {
		wxRemoveFile(answer);
		err = _("The engine could not be started.");
		return false;
	}
	if (result.timed_out) {
		wxRemoveFile(answer);
		wxString message = result.output;
		message.Trim();
		err = wxString::Format(_("The engine did not answer within %d seconds."),
							   (timeout_ms + 999) / 1000);
		if (!message.IsEmpty()) err += "\n\n" + message.Left(2000);
		return false;
	}

	std::string text;
	if (wxFileName::FileExists(answer)) {
		wxString read_err;
		ReadWholeFile(answer, text, read_err);
		wxRemoveFile(answer);
	}
	if (text.empty()) {
		wxString message = result.output;
		message.Trim();
		err = wxString::Format(_("The engine did not run (exit code %d):\n\n"),
							   result.exit_code) + message.Left(2000);
		return false;
	}
	output = wxString::FromUTF8(text.c_str());
	return true;
}

int RunJob(const wxString& engine_dir, const wxString& job_dir, wxString& output,
		   wxString& err, int timeout_sec, ProgressSink* sink)
{
	const wxString python = EnginePythonPath(engine_dir);
	if (python.IsEmpty()) {
		err = wxString::Format(_("There is no interpreter in %s."), engine_dir);
		return -1;
	}
	const wxString solver = SolverScriptPath();
	if (!wxFileName::FileExists(solver)) {
		err = wxString::Format(
			_("The solver is missing from this installation of GeoDa (%s)."), solver);
		return -1;
	}

	const wxString answer = job_dir + Sep() + "result.json";
	wxRemoveFile(answer);
	const wxString cmd = QuoteArg(python) + " " + QuoteArg(solver)
		+ " --job " + QuoteArg(job_dir);

	const ProcessResult result = RunProcess(cmd, answer, timeout_sec, sink);
	output = result.output;

	if (result.start_failed) {
		err = _("The engine could not be started.");
		return -1;
	}
	if (result.cancelled) {
		err = _("The estimation was cancelled.");
		return -1;
	}
	if (result.timed_out) {
		err = wxString::Format(_("The engine did not finish within %d seconds."),
							   timeout_sec);
		return -1;
	}
	if (!wxFileName::FileExists(answer)) {
		wxString message = output;
		message.Trim();
		err = wxString::Format(
			_("The engine stopped without producing a result (exit code %d):\n\n"),
			result.exit_code) + message.Left(2000);
		return -1;
	}
	// the solver's own verdict is in result.json; see PROTOCOL.md
	return 0;
}

} // namespace SpregEngine
