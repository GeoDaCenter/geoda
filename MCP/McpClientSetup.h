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

#ifndef __GEODA_CENTER_MCP_CLIENT_SETUP_H__
#define __GEODA_CENTER_MCP_CLIENT_SETUP_H__

#include <wx/arrstr.h>
#include <wx/string.h>
#include <vector>

/**
 * Install GeoDa into a coding agent (Claude Code, Codex), from a button in the
 * app.
 *
 * What "install" means here is the thing that decides whether the agent drives
 * GeoDa or works around it. The MCP server is registered as a **command** --
 * the stdio launcher this app ships -- and not as the http://127.0.0.1:8765/mcp
 * URL the MCP menu offers: registering a URL leaves one in the model's context,
 * and a client that cannot see the server's tools (one not restarted since
 * registering, or started while the app was down) will drive that endpoint by
 * hand, with hand-written JSON-RPC envelopes, tens of calls per session. As a
 * command the client starts the process itself and the model only ever sees
 * tools.
 *
 * The preferred route is the **plugin** the GeoDa project publishes: one
 * install carries the launcher, the MCP declaration and the skill, and the
 * client lists it in its own plugin/skill UI (in Codex the skill also lands in
 * the list `@` searches). The direct route below -- write the launcher, then
 * have the client register it -- is the fallback, used when the client cannot
 * install plugins or the plugin does not deliver the server to the client's own
 * `mcp get`.
 *
 * Everything here runs on the main thread: wxExecute is main-thread-only (it
 * asserts otherwise), so the caller is expected to show what is happening
 * before calling Install, which returns when the client's CLI has finished.
 */
class McpClientSetup
{
public:
	/** One line of the report the dialog shows. */
	struct Step {
		wxString label;
		wxString summary;
		wxString detail;
		bool ok;
		Step(const wxString& label_, bool ok_, const wxString& summary_,
		     const wxString& detail_)
		: label(label_), summary(summary_), detail(detail_), ok(ok_) {}
	};

	/** A client this app can install itself into, and where its CLI is. */
	struct Client {
		wxString id;		// "claude" or "codex"
		wxString label;		// "Claude Code" or "Codex"
		wxString binary;	// absolute path of the CLI; empty when not found
		wxArrayString searched;	// where it was looked for, for the message
		Client() : id(), label(), binary() {}
	};

	/** What happened, step by step. */
	struct Result {
		bool ok;
		bool restart_required;
		wxString client_label;
		std::vector<Step> steps;
		Result() : ok(false), restart_required(false), client_label() {}
	};

	/**
	 * Look for the CLI of each client this app knows how to install into.
	 * Returns one entry per client, in the order the dialog shows them, with
	 * `binary` empty and `searched` filled in for the ones that are missing.
	 */
	static std::vector<Client> ProbeClients();

	/**
	 * Install for one client, and report each step. `mcp_url` is the endpoint
	 * this instance of the app is actually serving, used only by the Windows
	 * fallback. Never throws; a failure is a step with ok == false.
	 */
	static Result Install(const Client& client, const wxString& mcp_url);

	/** The stdio launcher, as the app ships it (see MCP/bin/geoda-mcp). */
	static wxString LauncherScript();
	/** Where the direct route writes it: ~/.geoda/bin/geoda-mcp. */
	static wxString LauncherInstallPath();

	/** The marketplace the plugin is published from, and the plugin id. */
	static wxString Marketplace();
	static wxString PluginId();
	/** The name both clients register the MCP server under. */
	static wxString ServerName();
};

#endif
