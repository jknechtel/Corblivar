/*
 * =====================================================================================
 *
 *    Description:  Corblivar handler for timing, delay and power analysis
 *
 *    Copyright (C) 2015 Johann Knechtel, johann.knechtel@ifte.de, www.ifte.de
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
#ifndef _CORBLIVAR_TIMING_POWER
#define _CORBLIVAR_TIMING_POWER

// library includes
#include "Corblivar.incl.hpp"
// Corblivar includes, if any
// forward declarations, if any

class TimingPowerAnalyser {
	// debugging code switch (private)
	private:
		static constexpr bool DBG = false;

	// private constants
	private:
		// TSV/wire resistivity/capacitance values, taken from [Ahmed14] which
		// models 45nm technology; wires are on M7-M8 layers, TSV are assumed to
		// have 5um diameter, 10um pitch, and 50um length
		//
		// R_TSV [Ohm * 1e-3; mOhm]
		static constexpr double R_TSV =	42.8e-03;
		// C_TSV [F * 1e-15; fF]
		static constexpr double C_TSV = 28.664e-15;
		// R_wire [Ohm/um; mOhm/um]
		static constexpr double R_WIRE = 52.5e-03;
		// C_wire [F/um; fF/um]
		static constexpr double C_WIRE = 0.823e-15;

		// factor for modules' base delay [Lin10], [ns/um]; based on 90nm
		// technology simulations, thus scaled down by factor 2 to roughly match
		// 45nm technology; delay = factor times (width + height) for any module
		//
		static constexpr double DELAY_FACTOR_MODULE = 1.0/2000.0 / 2.0;
	
		// delay factors for TSVs and wires, taken/resulting from [Ahmed14] which
		// models 45nm technology
		//
		// TSVs' delay, given in [ns]
		static constexpr double DELAY_FACTOR_TSV = 
			// R_TSV [mOhm] * C_TSV [fF]
			R_TSV * C_TSV
			// scale up to ns
			* 1.0e09;
		// wire delay, given in [ns/um^2]
		static constexpr double DELAY_FACTOR_WIRE =
			// R_wire [mOhm/um] * C_wire [fF/um]
			R_WIRE * C_WIRE
			// scale up to ns
			* 1.0e09;

		// activity factor, taken from [Ahmed14]
		static constexpr double ACTIVITY_FACTOR = 0.1;

	// public POD, to be declared early on
	public:

	// private data, functions
	private:

	// constructors, destructors, if any non-implicit
	public:

	// public data, functions
	public:
		// module h and w shall be given in um; returned delay is in ns
		inline static double baseDelay(double const& h, double const& w) {
			return TimingPowerAnalyser::DELAY_FACTOR_MODULE * (h + w);
		}

		// WL shall be given in um; returned delay is in ns
		inline static double elmoreDelay(double const& WL, unsigned const& TSV) {
			return 0.5 * TimingPowerAnalyser::DELAY_FACTOR_WIRE * std::pow(WL, 2.0) + 0.5 * TimingPowerAnalyser::DELAY_FACTOR_TSV * std::pow(TSV, 2);
		}

		// WL shall be given in um; returned power is in W
		inline static double powerWire(double const& WL, double const& driver_voltage, double const& frequency, double const& activity_factor = TimingPowerAnalyser::ACTIVITY_FACTOR) {

			// P_wire = a * C_wire * WL * V_driver^2 * f
			// a * [F/um * um * V^2 * Hz] = [W]
			//
			return activity_factor * C_WIRE * WL * std::pow(driver_voltage, 2.0) * frequency;
		}

		inline static double powerTSV(double const& driver_voltage, double const& frequency, double const& activity_factor = TimingPowerAnalyser::ACTIVITY_FACTOR) {
			return activity_factor * C_TSV * std::pow(driver_voltage, 2.0) * frequency;
		}

	// private helper data, functions
	private:
};

#endif