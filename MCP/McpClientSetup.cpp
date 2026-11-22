/**
 * GeoDa TM, Copyright (C) 2011-2025 by Luc Anselin - all rights reserved
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

#include "MCP/McpClientSetup.h"

#include <wx/file.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/process.h>
#include <wx/stopwatch.h>
#include <wx/stream.h>
#include <wx/utils.h>

#include <string>
#include <vector>

#ifndef __WINDOWS__
#include <sys/stat.h>
#endif

namespace {

// The plugin, published from GeoDa's own repository -- .claude-plugin and
// .agents at its root, the plugin itself under plugins/geoda. Registering it
// gives the client the MCP server, the launcher that starts it, and the
// onboarding skill, all at once -- and the client can then list, update and
// remove the lot with its own commands.
//
// The app is a large repository (a fresh checkout runs to a few hundred MB) and
// the plugin needs three directories out of it, so the add is asked for a sparse
// checkout of exactly those. Both clients take the flag; a client old enough not
// to have it is retried without, which costs it the full clone but still works.
const char* kMarketplace = "GeoDaCenter/geoda";
const char* kMarketplaceClaudePaths = ".claude-plugin";
const char* kMarketplaceCodexPaths = ".agents";
const char* kMarketplacePluginPath = "plugins";
const char* kPluginName = "geoda";
const char* kPluginId = "geoda@geoda";
const char* kServerName = "geoda";

// The stdio launcher the direct route installs. The same script is committed at
// MCP/bin/geoda-mcp and shipped by the plugin at plugins/geoda/bin/geoda-mcp;
// MCP/tests/launcher_test.py fails when the copies drift apart.
const char* kLauncherScript = R"GEODALAUNCH(#!/bin/sh
# stdio MCP server for GeoDa.
#
# A client registers THIS as a command:
#
#   claude mcp add -s user geoda -- ~/.geoda/bin/geoda-mcp
#   codex  mcp add geoda -- ~/.geoda/bin/geoda-mcp
#
# Registering the app's HTTP endpoint by URL instead — `url = http://127.0.0.1:8765/mcp`
# — leaves a URL in the model's context, and a model that cannot see the server's
# tools (a client that was not restarted after registering, or that started while
# the app was down) will drive that endpoint by hand: hand-written JSON-RPC
# envelopes, headers and temp files, tens of calls per session. As a command the
# client starts this process and the model only ever sees tools.
#
# The script is a pass-through, and deliberately thin: one newline-delimited
# JSON-RPC message in on stdin, one POST to the app's loopback endpoint, one JSON
# reply out on stdout. Nothing else may go to stdout — it is the frame stream.
#
# Usage: geoda-mcp
#
# The app must be running. `~/.geoda/mcp.json`, which the app writes when its
# MCP server starts, carries the port that was actually bound (8765, or the next
# free port up to 8774, or an OS-assigned one); the pinned default below is only
# a fallback.

set -u

DEFAULT_URL='http://127.0.0.1:8765/mcp'
DISCOVERY_FILE="${GEODA_MCP_DISCOVERY_FILE:-$HOME/.geoda/mcp.json}"

url="${GEODA_MCP_URL:-}"
if [ -z "$url" ] && [ -r "$DISCOVERY_FILE" ]; then
  # No jq on a stock macOS, and the file is one small flat object, so read the
  # one key we need with sed. `head -1` because a file being rewritten could
  # briefly hold more than one match.
  url=$(sed -n 's/.*"url"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "$DISCOVERY_FILE" | head -1)
fi
if [ -z "$url" ]; then
  url="$DEFAULT_URL"
fi

# One JSON-RPC error reply, so a client that cannot reach the app gets a message
# it can show rather than a hang or a silent dead server. `id` may be empty (a
# notification), in which case nothing is printed — a reply to a notification is
# a protocol violation.
fail() {
  if [ -n "$id" ]; then
    printf '{"jsonrpc":"2.0","id":%s,"error":{"code":-32000,"message":"%s"}}\n' \
      "$id" "$1"
  fi
}

while IFS= read -r line; do
  # Blank line between frames is allowed by the transport; skip it.
  [ -n "$line" ] || continue

  id=$(printf '%s' "$line" |
    sed -n 's/.*"id"[[:space:]]*:[[:space:]]*\(-\{0,1\}[0-9][0-9]*\).*/\1/p' | head -1)

  if ! body=$(printf '%s' "$line" | curl -sS --max-time 1800 \
      -X POST "$url" \
      -H 'content-type: application/json' \
      -H 'accept: application/json, text/event-stream' \
      -H 'MCP-Protocol-Version: 2025-06-18' \
      --data-binary @- 2>&1); then
    fail "GeoDa is not reachable at $url - start GeoDa (it serves MCP while it runs) and retry. $body"
    continue
  fi

  # A notification is answered 202 with no body: nothing to pass on.
  [ -n "$body" ] || continue
  printf '%s\n' "$body"
done

# stdin closed: the client has gone away, so exit and let the app reap the
# session. Nothing to clean up — every call was independent.
exit 0
)GEODALAUNCH";

/** Output of one client CLI invocation. */
struct CommandResult {
	long code;		// exit code, or -1 when it could not be started
	bool started;
	wxString out;	// stdout and stderr, as the CLI printed them
	CommandResult() : code(-1), started(false) {}
};

typedef std::vector<wxString> StringList;

/**
 * <dir>/<relative>, as a path. wxFileName(dir, name) is the obvious spelling but
 * its second argument is a *file name*: given "/bin/claude" it asserts and is
 * not defined to produce what was meant.
 */
wxString Under(const wxString& dir, const wxString& relative)
{
	return dir + wxFileName::GetPathSeparator() + relative;
}

/**
 * The argument list of one client command, in order.
 *
 * Built by hand rather than with a brace-initialised list: the Windows projects
 * in this repository include one that predates them, and there is no reason for
 * a shell-out to be the thing that needs a newer compiler.
 */
StringList Args1(const wxString& a1)
{
	StringList args;
	args.push_back(a1);
	return args;
}

StringList Args2(const wxString& a1, const wxString& a2)
{
	StringList args = Args1(a1);
	args.push_back(a2);
	return args;
}

StringList Args3(const wxString& a1, const wxString& a2, const wxString& a3)
{
	StringList args = Args2(a1, a2);
	args.push_back(a3);
	return args;
}

StringList Args4(const wxString& a1, const wxString& a2, const wxString& a3,
                 const wxString& a4)
{
	StringList args = Args3(a1, a2, a3);
	args.push_back(a4);
	return args;
}

// Where each client's CLI is looked for, in order. `codex` is routinely NOT on
// PATH: the ChatGPT app ships it inside its own bundle, and that location has
// already moved once (Contents/Resources/codex -> Contents/Resources/codex-cli/
// bin/codex), so both are tried. Absolute paths rather than a wildcard under
// /Applications, which would pick up whatever else happens to be installed
// there.
void AppendClaudeCandidates(StringList* out)
{
	out->push_back("claude");
	out->push_back(Under(wxGetHomeDir(), ".local/bin/claude"));
	out->push_back("/usr/local/bin/claude");
	out->push_back("/opt/homebrew/bin/claude");
}

void AppendCodexCandidates(StringList* out)
{
	out->push_back("codex");
	out->push_back(
	    "/Applications/ChatGPT.app/Contents/Resources/codex-cli/bin/codex");
	out->push_back("/Applications/ChatGPT.app/Contents/Resources/codex");
	out->push_back(Under(wxGetHomeDir(),
	                      "Applications/ChatGPT.app/Contents/Resources/"
	                      "codex-cli/bin/codex"));
	out->push_back(Under(wxGetHomeDir(), ".codex/bin/codex"));
}

bool FileExists(const wxString& path)
{
	return !path.IsEmpty() && wxFileName::FileExists(path);
}

/** True when the CLI's output mentions "already". */
bool MentionsAlready(const wxString& out)
{
	return out.Lower().Find("already") != wxNOT_FOUND;
}

/** The CLI's output, trimmed to something a dialog can show. */
wxString FirstLines(const wxString& out)
{
	wxString trimmed = out;
	trimmed.Trim(true);
	trimmed.Trim(false);
	if (trimmed.length() > 400) trimmed = trimmed.Left(400) + "...";
	return trimmed;
}

/** True for a line that looks like a path, not a shell banner. */
bool LooksLikePath(const wxString& line)
{
	return line.StartsWith("/") || (line.length() > 1 && line[1] == ':');
}

#ifndef __WINDOWS__
// A client CLI is given this long before it is stopped and the direct route
// takes over. `plugin marketplace add` clones a repository, which a slow
// connection can drag out, but not for five minutes.
const long kClientTimeoutMs = 5 * 60 * 1000;

/**
 * The process object for a client CLI.
 *
 * wx's own OnTerminate is what deletes the object, and wx's bookkeeping for a
 * child reaches into it from the event loop while the child is reaped -- freeing
 * it first is a crash inside wxExecuteData::OnExit(). So the two facts the loop
 * below needs are recorded here instead, and the object is left to the code that
 * started it. GeoDa's solver runner does the same, for the same reason (see
 * Regression/SpregEngine.cpp).
 */
class ClientProcess : public wxProcess {
public:
	ClientProcess() : wxProcess(wxPROCESS_REDIRECT), terminated(false), status(-1) {}

	virtual void OnTerminate(int WXUNUSED(pid), int status_in)
	{
		status = status_in;
		terminated = true;
	}

	bool terminated;
	int status;
};

/** Read what both pipes hold right now, appending it to `out`. */
void DrainProcess(ClientProcess* proc, wxString* out)
{
	wxInputStream* streams[2] = { proc->GetInputStream(), proc->GetErrorStream() };
	char buffer[4096];
	for (int i = 0; i < 2; ++i) {
		wxInputStream* stream = streams[i];
		while (stream && stream->CanRead()) {
			stream->Read(buffer, sizeof(buffer));
			const size_t n = stream->LastRead();
			if (n == 0) break;	// the child closed this one
			*out += wxString::FromUTF8(buffer, n);
		}
	}
}

/**
 * Run a client CLI and capture what it printed.
 *
 * argv form, no shell: the binary is already an absolute path and the arguments
 * (a marketplace, a plugin id, a path) are handed over as they are, so nothing
 * in them can be re-parsed. The wxString form is NOT used on Unix -- it splits
 * the command line itself and never reaches a shell, so quotes and redirects
 * written there would quietly do nothing. Its capture overload, the one that
 * drains both pipes for you, exists for that form only.
 *
 * The child has to be drained while it is alive: a command that prints more than
 * a pipe buffer holds (64 KB) blocks on its next write, and against a
 * synchronous wait for it to exit that is a deadlock, on the GUI thread -- and
 * `plugin marketplace add` is a git clone, which likes to talk. So it runs
 * asynchronously, with both streams read as they fill, until it goes away or
 * until it has had long enough. wxExecute runs on the main thread only -- it
 * asserts otherwise -- which is why the dialog says what it is doing first, and
 * why it refuses a second install while this one is on the stack: the wait turns
 * the event loop, so a click that arrives during it is dispatched.
 */
CommandResult RunArgv(const wxString& program, const StringList& args)
{
	CommandResult result;

	// A program that is not there is something an asynchronous start cannot
	// report: the fork succeeds and the child dies in exec, which is
	// indistinguishable from a command that ran and failed. The callers tell
	// those apart -- "the client is not installed" is not "the client refused"
	// -- so it is settled before anything is started.
	if (!FileExists(program)) return result;

	std::vector<std::string> utf8;
	utf8.push_back(std::string(program.utf8_str()));
	for (size_t i = 0; i < args.size(); ++i) {
		utf8.push_back(std::string(args[i].utf8_str()));
	}
	std::vector<const char*> argv;
	for (size_t i = 0; i < utf8.size(); ++i) argv.push_back(utf8[i].c_str());
	argv.push_back(NULL);

	ClientProcess* proc = new ClientProcess();
	const long pid = wxExecute(&argv[0], wxEXEC_ASYNC, proc);
	if (pid == 0) {
		delete proc;
		return result;		// never started: code stays -1
	}
	result.started = true;

	wxStopWatch watch;
	for (;;) {
		DrainProcess(proc, &result.out);
		if (proc->terminated || !wxProcess::Exists(static_cast<int>(pid))) break;

		if (watch.Time() > kClientTimeoutMs) {
			// At the front, not appended: what a dialog is shown is the head of
			// this (see FirstLines).
			result.out = wxString::Format(
			    _("(the client was stopped after %d minutes)\n"),
			    static_cast<int>(kClientTimeoutMs / 60000)) + result.out;
			wxLogMessage("MCP client setup: %s timed out after %ld ms", program,
			             kClientTimeoutMs);
			wxProcess::Kill(static_cast<int>(pid), wxSIGTERM, wxKILL_CHILDREN);
			for (int i = 0; i < 20 && !proc->terminated &&
			                wxProcess::Exists(static_cast<int>(pid)); ++i) {
				wxMilliSleep(100);
				wxYieldIfNeeded();
			}
			if (!proc->terminated && wxProcess::Exists(static_cast<int>(pid))) {
				wxProcess::Kill(static_cast<int>(pid), wxSIGKILL, wxKILL_CHILDREN);
			}
			break;
		}

		wxMilliSleep(50);
		// wx reaps the child from the event loop, and the exit status arrives
		// with it, so the loop has to turn for this to ever finish.
		wxYieldIfNeeded();
	}
	DrainProcess(proc, &result.out);

	result.code = proc->terminated ? proc->status : -1;
	if (proc->terminated || !wxProcess::Exists(static_cast<int>(pid))) {
		delete proc;
	} else {
		// A child that outlived SIGKILL is not something to free while wx still
		// has it; leave it to wx, as the solver runner does.
		wxLogMessage("MCP client setup: %s is still running", program);
		proc->Detach();
	}
	return result;
}
#endif

#ifdef __WINDOWS__
/** Windows command line: quote anything with a space in it, /bin/sh style. */
wxString QuoteWin(const wxString& s)
{
	return (s.Find(' ') == wxNOT_FOUND) ? s : "\"" + s + "\"";
}
#endif

/** Run one client CLI and capture its output. */
CommandResult RunClient(const wxString& program, const StringList& args)
{
#ifdef __WINDOWS__
	// Windows has no /bin/sh, and the clients there are .cmd shims that want a
	// native command line, so the string form -- which CreateProcess takes
	// verbatim -- is the right one here.
	CommandResult result;
	wxString cmdline = QuoteWin(program);
	for (size_t i = 0; i < args.size(); ++i) cmdline += " " + QuoteWin(args[i]);
	wxArrayString out;
	wxArrayString err;
	result.code = wxExecute(cmdline, out, err, wxEXEC_SYNC);
	result.started = (result.code >= 0);
	result.out = wxJoin(out, '\n');
	if (!err.IsEmpty()) result.out += "\n" + wxJoin(err, '\n');
	return result;
#else
	return RunArgv(program, args);
#endif
}

/** `command -v <name>` -- or `where <name>` on Windows -- as one argv. */
CommandResult RunPathProbe(const wxString& name)
{
#ifdef __WINDOWS__
	return RunClient("cmd.exe", Args2("/c", "where " + name));
#else
	// A *login* shell: a GUI app started from Finder has a minimal PATH that
	// does not include the directories clients are usually installed into,
	// while the login shell's PATH does.
	return RunClient("/bin/sh", Args2("-lc", "command -v " + name));
#endif
}

/**
 * Does the client know a GeoDa server at all, whoever registered it?
 *
 * A server that arrived with a plugin is *not* under the name the app would
 * register by hand: Claude Code lists it as `plugin:<plugin>:<server>`, and
 * `mcp get geoda` then fails even though the client has the server and is
 * connected to it. Both names are tried, and the bare one first, since that is
 * the one a direct registration uses (and the only one Codex uses). This is the
 * question the direct route asks, where any registration is one to leave alone.
 */
bool ServerIsRegistered(const wxString& binary)
{
	if (RunClient(binary, Args3("mcp", "get", kServerName)).code == 0) {
		return true;
	}
	const wxString qualified =
	    wxString::Format("plugin:%s:%s", kPluginName, kServerName);
	return RunClient(binary, Args3("mcp", "get", qualified)).code == 0;
}

/**
 * Did the plugin actually deliver the server? -- a different question, and the
 * only one the plugin route may answer yes to.
 *
 * Asking the loose question above would let a registration the user made by hand
 * stand in for the plugin's: someone who set GeoDa up the old way has a bare
 * `geoda`, so a plugin that delivered nothing would look like it had worked, the
 * dialog would report the server as coming from the plugin, and the direct route
 * that would have fixed it would be skipped.
 *
 * On Claude Code the plugin's server is the qualified name and a bare `geoda` is
 * therefore a hand-made registration, so only the qualified name counts. Codex
 * files both under the bare name, so the reply has to name this plugin's own
 * launcher rather than merely resolve.
 */
bool PluginDeliveredServer(const McpClientSetup::Client& client)
{
	if (client.id == "claude") {
		const wxString qualified =
		    wxString::Format("plugin:%s:%s", kPluginName, kServerName);
		return RunClient(client.binary, Args3("mcp", "get", qualified)).code == 0;
	}
	CommandResult known =
	    RunClient(client.binary, Args3("mcp", "get", kServerName));
	return known.code == 0 && known.out.Find("geoda-mcp") != wxNOT_FOUND;
}

/**
 * The plugin route: register the marketplace, install the plugin, then read the
 * registration back.
 *
 * Both commands are idempotent enough to re-run -- an already-added marketplace
 * and an already-installed plugin are successes, not failures -- and both say
 * so on stdout rather than by exit code, so the messages are read, not the
 * status. Any other failure (an older CLI with no `plugin` subcommand, no
 * network for the marketplace) is not something the user needs to see as an
 * error: it returns false, and the direct route installs the same server
 * without the plugin.
 *
 * Windows never gets this far: the server a plugin registers is the launcher,
 * and a client there cannot run a POSIX shell script -- so the install would
 * succeed, the read-back would resolve, and the user would be left with tools
 * that never start.
 */
bool InstallViaPlugin(const McpClientSetup::Client& client,
                      std::vector<McpClientSetup::Step>* steps,
                      wxString* why_not)
{
#ifdef __WINDOWS__
	(void) steps;
	*why_not = _("The plugin registers GeoDa's MCP launcher, which is a POSIX "
	             "shell script: a client on Windows cannot run it, so the plugin "
	             "is not installed there and the HTTP endpoint is registered "
	             "instead.");
	return false;
#endif
	const bool claude = (client.id == "claude");

	// Claude Code takes one --sparse and then every path; Codex takes a --sparse
	// per path. The paths are the three directories the plugin is made of: the
	// two marketplace files at the repository root, and the plugin itself.
	StringList add = Args4("plugin", "marketplace", "add", kMarketplace);
	if (claude) {
		add.push_back("--sparse");
		add.push_back(kMarketplaceClaudePaths);
		add.push_back(kMarketplaceCodexPaths);
		add.push_back(kMarketplacePluginPath);
	} else {
		add.push_back("--sparse");
		add.push_back(kMarketplaceClaudePaths);
		add.push_back("--sparse");
		add.push_back(kMarketplaceCodexPaths);
		add.push_back("--sparse");
		add.push_back(kMarketplacePluginPath);
	}

	CommandResult market = RunClient(client.binary, add);
	if (!market.started) {
		*why_not = wxString::Format(_("%s could not be run."), client.label);
		return false;
	}
	// The same add without the sparse paths, for a client whose CLI predates the
	// flag: it clones the whole app repository to get the same three directories.
	bool sparse_dropped = false;
	if (market.code != 0 && !MentionsAlready(market.out)) {
		CommandResult retry = RunClient(
		    client.binary, Args4("plugin", "marketplace", "add", kMarketplace));
		if (retry.started && (retry.code == 0 || MentionsAlready(retry.out))) {
			market = retry;
			sparse_dropped = true;
		}
	}
	if (market.code != 0 && !MentionsAlready(market.out)) {
		*why_not = wxString::Format(_("`plugin marketplace add %s` failed: %s"),
		                            kMarketplace, FirstLines(market.out));
		return false;
	}

	// `-y` on Claude Code: the marketplace declares a command for the client to
	// run, and its installer asks before trusting one. This is the button the
	// user already pressed, for a plugin the GeoDa project publishes.
	CommandResult install =
	    claude ? RunClient(client.binary,
	                       Args4("plugin", "install", "-y", kPluginId))
	           : RunClient(client.binary, Args3("plugin", "add", kPluginId));
	if (!install.started) {
		*why_not = wxString::Format(_("%s could not be run."), client.label);
		return false;
	}
	const bool already = MentionsAlready(install.out);
	if (install.code != 0 && !already) {
		*why_not = wxString::Format(_("`plugin install %s` failed: %s"),
		                            kPluginId, FirstLines(install.out));
		return false;
	}

	// Read the registration back rather than trusting the install: a marketplace
	// copy whose plugin declares no MCP server installs cleanly and leaves the
	// client with no tools at all -- the silent failure this route exists to
	// avoid -- so the direct route takes over when the plugin's own server is
	// not there. The plugin's own, not just any: see PluginDeliveredServer.
	if (!PluginDeliveredServer(client)) {
		*why_not = wxString::Format(
		    _("the plugin installed but %s does not see the plugin's `%s` MCP "
		      "server (`mcp get` does not resolve it to this plugin), so the "
		      "direct route is used instead"),
		    client.label, kServerName);
		return false;
	}

	wxString where =
	    wxString::Format(_("%s is now a known marketplace for %s."),
	                     kMarketplace, client.label);
	where += sparse_dropped
	             ? _(" This client's CLI has no `--sparse`, so the whole "
	                 "repository was checked out to get it.")
	             : _(" Only the plugin's own directories were checked out.");
	steps->push_back(McpClientSetup::Step(
	    _("Plugin marketplace"), true, _("registered"), where));
	steps->push_back(McpClientSetup::Step(
	    _("Plugin"), true, already ? _("already installed") : _("installed"),
	    wxString::Format(
	        _("%s -- brings the MCP server (as a local command), the launcher "
	          "that starts it, and the skill that sets the client up and drives "
	          "the app, so nothing else has to be installed."),
	        kPluginId)));
	steps->push_back(McpClientSetup::Step(
	    _("MCP server"), true, _("from the plugin"),
	    wxString::Format(_("`mcp get %s` resolves it, so %s sees GeoDa's "
	                       "commands as tools once it is restarted."),
	                     kServerName, client.label)));
	return true;
}

/**
 * Write the launcher to ~/.geoda/bin/geoda-mcp.
 *
 * One stable path, so that a reinstall from either button overwrites the same
 * file, and because a registration pointing at a file the user can read is
 * easier to explain than one pointing into an app bundle.
 */
bool InstallLauncher(wxString* path, wxString* error)
{
	const wxString launcher_path = McpClientSetup::LauncherInstallPath();
	const wxString dir = wxFileName(launcher_path).GetPath();

	if (!wxFileName::DirExists(dir) &&
	    !wxFileName::Mkdir(dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
		*error = wxString::Format(_("Could not create the directory %s."), dir);
		return false;
	}

	wxFile file;
	if (!file.Create(launcher_path, true) &&
	    !file.Open(launcher_path, wxFile::write)) {
		*error = wxString::Format(_("Could not write %s."), launcher_path);
		return false;
	}
	// The script has to outlive the buffer its UTF-8 form points at:
	// `LauncherScript().utf8_str()` hands back a *view* into a temporary, which
	// is gone by the time the bytes are written -- and the failure is a file
	// with garbage where its first line should be, not a crash.
	const wxString script = McpClientSetup::LauncherScript();
	const wxScopedCharBuffer utf8 = script.utf8_str();
	if (file.Write(utf8.data(), utf8.length()) != (size_t) utf8.length()) {
		file.Close();
		*error = wxString::Format(_("Could not write %s."), launcher_path);
		return false;
	}
	file.Close();

#ifndef __WINDOWS__
	// The client runs it, so it has to be executable. Windows has no /bin/sh to
	// run it with and takes the URL route instead (see Install).
	chmod(launcher_path.utf8_str(), 0755);
#endif

	*path = launcher_path;
	return true;
}

}  // namespace

wxString McpClientSetup::Marketplace() { return kMarketplace; }

wxString McpClientSetup::PluginId() { return kPluginId; }

wxString McpClientSetup::ServerName() { return kServerName; }

wxString McpClientSetup::LauncherScript()
{
	return wxString(kLauncherScript, wxConvUTF8);
}

wxString McpClientSetup::LauncherInstallPath()
{
	return Under(wxGetHomeDir(), ".geoda/bin/geoda-mcp");
}

std::vector<McpClientSetup::Client> McpClientSetup::ProbeClients()
{
	std::vector<Client> clients;

	const char* ids[2] = { "claude", "codex" };
	const char* labels[2] = { "Claude Code", "Codex" };

	for (int i = 0; i < 2; ++i) {
		Client client;
		client.id = ids[i];
		client.label = labels[i];

		StringList candidates;
		if (client.id == "claude") {
			AppendClaudeCandidates(&candidates);
		} else {
			AppendCodexCandidates(&candidates);
		}

		// An environment override wins outright, rather than being tried first:
		// an unusual install (a beta build in a temporary directory, a CI image)
		// should not need a code change, and pointing the variable at a path
		// that does not exist is how the "client not installed" case is
		// exercised -- it has to mean that, not "look elsewhere as well".
		wxString override_path;
		if (wxGetEnv(client.id == "claude" ? "GEODA_CLAUDE_BIN" : "GEODA_CODEX_BIN",
		             &override_path) &&
		    !override_path.IsEmpty()) {
			candidates.clear();
			candidates.push_back(override_path);
		}

		for (size_t j = 0; j < candidates.size() && client.binary.IsEmpty();
		     ++j) {
			const wxString& candidate = candidates[j];
			client.searched.Add(candidate);

			if (wxIsAbsolutePath(candidate)) {
				if (FileExists(candidate)) client.binary = candidate;
				continue;
			}

			// A bare name: ask the platform, since PATH differs between the
			// app's environment (started from Finder) and a terminal's.
			CommandResult probe = RunPathProbe(candidate);
			if (probe.code != 0) continue;

			// A login shell can print a banner before the answer, so take the
			// first line that is a path rather than the first line there is.
			wxString found;
			size_t pos = 0;
			while (pos <= probe.out.length()) {
				wxString line = probe.out.Mid(pos).BeforeFirst('\n');
				line.Trim(true);
				line.Trim(false);
				if (LooksLikePath(line)) {
					found = line;
					break;
				}
				size_t nl = probe.out.find('\n', pos);
				if (nl == wxString::npos) break;
				pos = nl + 1;
			}
			if (FileExists(found)) client.binary = found;
		}

		clients.push_back(client);
	}
	return clients;
}

McpClientSetup::Result McpClientSetup::Install(const Client& client,
                                               const wxString& mcp_url)
{
	Result result;
	result.client_label = client.label;

	if (client.binary.IsEmpty()) {
		result.steps.push_back(Step(
		    _("MCP server"), false, _("client not found"),
		    wxString::Format(_("%s was not found, so GeoDa was not registered "
		                       "with it. Looked in: %s"),
		                     client.label, wxJoin(client.searched, ','))));
		return result;
	}

	// The plugin first: it is the whole install in two commands, and what the
	// client shows in its own plugin and skill lists afterwards.
	wxString why_not;
	std::vector<Step> plugin_steps;
	if (InstallViaPlugin(client, &plugin_steps, &why_not)) {
		result.ok = true;
		result.restart_required = true;
		result.steps = plugin_steps;
		wxLogMessage("MCP client setup: %s installed from the plugin", client.id);
		return result;
	}
	// Not fatal -- the direct route below installs the same server. Say why the
	// plugin route was not used rather than hiding it.
	result.steps.push_back(Step(_("Plugin"), true, _("not used"), why_not));

	// 1. The launcher.
	wxString launcher;
	wxString error;
#ifdef __WINDOWS__
	// The launcher is a POSIX shell script and Windows has no /bin/sh to run it
	// with, so the direct route registers the HTTP endpoint there instead. That
	// is the shape this button is moving away from; it is the only one that
	// works on Windows, and the step says so.
	result.steps.push_back(Step(
	    _("Launcher"), true, _("not used on Windows"),
	    _("The MCP launcher is a shell script and Windows has no /bin/sh for it. "
	      "The endpoint is registered as a URL instead: that works, but it "
	      "leaves a URL in the agent's context, and the agent can only see "
	      "GeoDa's tools if the client was restarted after this and GeoDa is "
	      "running.")));
#else
	if (InstallLauncher(&launcher, &error)) {
		result.steps.push_back(
		    Step(_("Launcher"), true, _("installed"), launcher));
	} else {
		result.steps.push_back(Step(_("Launcher"), false, _("failed"), error));
	}
#endif

	// 2. The registration -- as a command, so the client owns the transport.
	//    An existing registration is left alone: replacing one the user set up
	//    by hand is not this button's business.
	if (ServerIsRegistered(client.binary)) {
		result.steps.push_back(Step(
		    _("MCP server"), true, _("already registered"),
		    wxString::Format(_("%s already knows a `%s` server, and it is left "
		                       "as it is. Run `mcp remove %s` in %s first to "
		                       "replace it with the one installed here."),
		                     client.label, kServerName, kServerName,
		                     client.label)));
		result.ok = true;
		return result;
	}

	const bool claude = (client.id == "claude");
	CommandResult added;
#ifdef __WINDOWS__
	{
		// `-s user` on Claude Code, not the default `local` scope (see below).
		StringList args = Args2("mcp", "add");
		if (claude) {
			args.push_back("-s");
			args.push_back("user");
			args.push_back("--transport");
			args.push_back("http");
		}
		args.push_back(kServerName);
		if (!claude) args.push_back("--url");
		args.push_back(mcp_url);
		added = RunClient(client.binary, args);
	}
	const wxString how = _("registered (HTTP URL)");
	const wxString detail = wxString::Format(
	    _("%s now connects to %s. There is no launcher on Windows, so this is "
	      "the HTTP endpoint: an agent that cannot see GeoDa's tools (the "
	      "client was not restarted, or GeoDa is not running) will fall back "
	      "to calling that URL by hand."),
	    client.label, mcp_url);
#else
	if (launcher.IsEmpty()) {
		// The launcher could not be written, so there is nothing to register.
		result.steps.push_back(Step(
		    _("MCP server"), false, _("not registered"),
		    _("The launcher is not in place, so there is nothing for the "
		      "client to run.")));
		return result;
	}
	// `-s user` on Claude Code, not the default `local`: a local registration
	// applies to one project directory only, so the button would appear to work
	// and then not apply where the user actually runs the client.
	{
		StringList args = Args2("mcp", "add");
		if (claude) {
			args.push_back("-s");
			args.push_back("user");
		}
		args.push_back(kServerName);
		args.push_back("--");
		args.push_back(launcher);
		added = RunClient(client.binary, args);
	}
	const wxString how = _("registered");
	const wxString detail = wxString::Format(
	    _("%s now runs %s: a stdio server, so the client owns the connection "
	      "and the agent sees GeoDa's commands as tools. The skill is served "
	      "by GeoDa itself (`initialize` and `skill/get`), so there is no "
	      "skill file to install."),
	    client.label, launcher);
#endif

	if (added.started && added.code == 0) {
		result.steps.push_back(Step(_("MCP server"), true, how, detail));
	} else {
		result.steps.push_back(Step(
		    _("MCP server"), false, _("failed"),
		    wxString::Format(
		        _("`mcp add` failed (%s): %s"),
		        added.started ? wxString::Format("%ld", added.code)
		                      : _("could not run the client"),
		        FirstLines(added.out))));
		return result;
	}

	result.ok = true;
	result.restart_required = true;
	wxLogMessage("MCP client setup: %s installed directly (launcher)", client.id);
	return result;
}
