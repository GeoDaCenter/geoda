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

#ifndef __GEODA_CENTER_MCP_COMMANDS_H__
#define __GEODA_CENTER_MCP_COMMANDS_H__

class McpTools;

// Register every GeoDa command (menu actions and analysis tools) with the
// MCP tool registry. This is the single source of truth for the app's
// command surface: each menu item maps to a command with a command id, a
// human-readable label, a menu grouping, a JSON schema, and a handler that
// reads its arguments from JSON (no dialogs). Called once from
// McpTools::RegisterTools(); the registry is read-only afterwards.
void RegisterCommands(McpTools& tools);

#endif
