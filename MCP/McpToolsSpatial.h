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

#ifndef __GEODA_CENTER_MCP_TOOLS_SPATIAL_H__
#define __GEODA_CENTER_MCP_TOOLS_SPATIAL_H__

#include "MCP/McpTools.h"

// Tool handlers. Each takes the tool context and the JSON-RPC "arguments"
// object, and returns the tool's result as a json_spirit::Value. Expected
// errors are signaled by throwing McpError.

// project
json_spirit::Value McpProjectStatus(const McpToolContext& ctx,
                                    const json_spirit::Object& params);

// table
json_spirit::Value McpTableListColumns(const McpToolContext& ctx,
                                       const json_spirit::Object& params);
json_spirit::Value McpTableGetColumn(const McpToolContext& ctx,
                                     const json_spirit::Object& params);
json_spirit::Value McpTableUnivariateStats(const McpToolContext& ctx,
                                          const json_spirit::Object& params);

// file
json_spirit::Value McpFileExport(const McpToolContext& ctx,
                                 const json_spirit::Object& params);
json_spirit::Value McpTableExport(const McpToolContext& ctx,
                                  const json_spirit::Object& params);

// weights
json_spirit::Value McpWeightsCreate(const McpToolContext& ctx,
                                    const json_spirit::Object& params);
json_spirit::Value McpWeightsList(const McpToolContext& ctx,
                                  const json_spirit::Object& params);
json_spirit::Value McpWeightsDescribe(const McpToolContext& ctx,
                                      const json_spirit::Object& params);

// lisa
json_spirit::Value McpLisaLocalMoran(const McpToolContext& ctx,
                                     const json_spirit::Object& params);
json_spirit::Value McpLisaLocalGeary(const McpToolContext& ctx,
                                     const json_spirit::Object& params);
json_spirit::Value McpLisaLocalG(const McpToolContext& ctx,
                                 const json_spirit::Object& params);

// global
json_spirit::Value McpGlobalMoran(const McpToolContext& ctx,
                                  const json_spirit::Object& params);
json_spirit::Value McpGlobalGeary(const McpToolContext& ctx,
                                  const json_spirit::Object& params);
json_spirit::Value McpGlobalGeneralG(const McpToolContext& ctx,
                                     const json_spirit::Object& params);

// cluster
json_spirit::Value McpClusterSkater(const McpToolContext& ctx,
                                    const json_spirit::Object& params);
json_spirit::Value McpClusterRedcap(const McpToolContext& ctx,
                                    const json_spirit::Object& params);
json_spirit::Value McpClusterSchc(const McpToolContext& ctx,
                                  const json_spirit::Object& params);
json_spirit::Value McpClusterMaxp(const McpToolContext& ctx,
                                  const json_spirit::Object& params);
json_spirit::Value McpClusterAzp(const McpToolContext& ctx,
                                 const json_spirit::Object& params);
json_spirit::Value McpClusterSpatialKmeans(const McpToolContext& ctx,
                                           const json_spirit::Object& params);
json_spirit::Value McpClusterDbscan(const McpToolContext& ctx,
                                    const json_spirit::Object& params);
json_spirit::Value McpClusterHdbscan(const McpToolContext& ctx,
                                     const json_spirit::Object& params);
json_spirit::Value McpClusterSpectral(const McpToolContext& ctx,
                                      const json_spirit::Object& params);
json_spirit::Value McpClusterPam(const McpToolContext& ctx,
                                 const json_spirit::Object& params);
json_spirit::Value McpClusterMds(const McpToolContext& ctx,
                                 const json_spirit::Object& params);
json_spirit::Value McpClusterPca(const McpToolContext& ctx,
                                 const json_spirit::Object& params);
json_spirit::Value McpClusterKmeans(const McpToolContext& ctx,
                                    const json_spirit::Object& params);
json_spirit::Value McpClusterKmedians(const McpToolContext& ctx,
                                      const json_spirit::Object& params);
json_spirit::Value McpClusterHierarchical(const McpToolContext& ctx,
                                          const json_spirit::Object& params);

// regress
json_spirit::Value McpRegressClassic(const McpToolContext& ctx,
                                     const json_spirit::Object& params);

// window
json_spirit::Value McpWindowCreateMap(const McpToolContext& ctx,
                                      const json_spirit::Object& params);
json_spirit::Value McpWindowCreatePlot(const McpToolContext& ctx,
                                       const json_spirit::Object& params);
json_spirit::Value McpWindowCreateLisaMap(const McpToolContext& ctx,
                                          const json_spirit::Object& params);
json_spirit::Value McpWindowCreateScatterPlotMatrix(const McpToolContext& ctx,
                                                    const json_spirit::Object& params);
json_spirit::Value McpWindowCreateBubbleChart(const McpToolContext& ctx,
                                              const json_spirit::Object& params);
json_spirit::Value McpWindowCreate3DScatter(const McpToolContext& ctx,
                                            const json_spirit::Object& params);
json_spirit::Value McpWindowCreatePcp(const McpToolContext& ctx,
                                     const json_spirit::Object& params);
json_spirit::Value McpWindowCreateLineChart(const McpToolContext& ctx,
                                           const json_spirit::Object& params);
json_spirit::Value McpWindowCreateCorrelogram(const McpToolContext& ctx,
                                              const json_spirit::Object& params);
json_spirit::Value McpWindowCreateDistancePlot(const McpToolContext& ctx,
                                               const json_spirit::Object& params);
json_spirit::Value McpWindowCreateMoranScatterplot(const McpToolContext& ctx,
                                                   const json_spirit::Object& params);

#endif
