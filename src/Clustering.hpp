/*
 * =====================================================================================
 *
 *    Description:  Corblivar signal-TSV clustering
 *
 *    Copyright (C) 2013 Johann Knechtel, johann.knechtel@ifte.de, www.ifte.de
 *
 *    This file is part of Corblivar.
 *    
 *    Corblivar is free software: you can redistribute it and/or modify it under the terms
 *    of the GNU General Public License as published by the Free Software Foundation,
 *    either version 3 of the License, or (at your option) any later version.
 *    
 *    Corblivar is distributed in the hope that it will be useful, but WITHOUT ANY
 *    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 *    PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *    
 *    You should have received a copy of the GNU General Public License along with
 *    Corblivar.  If not, see <http://www.gnu.org/licenses/>.
 *
 * =====================================================================================
 */
#ifndef _CORBLIVAR_CLUSTERING
#define _CORBLIVAR_CLUSTERING

// library includes
#include "Corblivar.incl.hpp"
// Corblivar includes, if any
#include "Net.hpp"
#include "ThermalAnalyzer.hpp"
// forward declarations, if any

class Clustering {
	// debugging code switches
	private:
		static constexpr bool DBG = false;
		static constexpr bool DBG_HOTSPOT = false;
		static constexpr bool DBG_CLUSTERING = false;
	public:
		static constexpr bool DBG_HOTSPOT_PLOT = false;

	// constructors, destructors, if any non-implicit
	public:

	// public data, functions
	public:
		void clusterSignalTSVs(vector< list<Net::Segments> > &nets_seg, ThermalAnalyzer::ThermalAnalysisResult thermal_analysis);

	// private data, functions
	private:

		// POD for hotspot regions
		struct HotspotRegion {
			double peak_temp;
			double base_temp;
			double temp_gradient;
			list<ThermalAnalyzer::ThermalMapBin*> bins;
			bool still_growing;
			int region_id;
			double region_score;
		};
		// related container
		map<int, HotspotRegion> hotspot_regions;

		// hotspot determination
		void determineHotspots(ThermalAnalyzer::ThermalAnalysisResult thermal_analysis);
};

#endif
