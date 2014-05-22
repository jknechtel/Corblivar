Licence
=======

Copyright (C) 2013 Johann Knechtel, johann.knechtel@ifte.de, www.ifte.de

This file is part of Corblivar.

Corblivar is a simulated-annealing-based floorplanning suite for 3D ICs, with special
emphasis on structural planning of massive interconnects by block alignment.

Corblivar is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Corblivar is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
Corblivar.  If not, see <http://www.gnu.org/licenses/>.

Citation
========
If you find this tool useful, and apply it for your research and publications, please
cite our paper:
Knechtel, J.; Young, E. F. Y. & Lienig, J. "Structural Planning of 3D-IC Interconnects by
Block Alignment" Proc. Asia South Pacific Des. Autom. Conf., 2014, 53-60

To compile and run Corblivar, you need the following tools
==========================================================
- clang++ (at least v 3.1 is required; compiling w/ version 3.2 was tested)
- gnuplot
- octave
- perl
- cairosvg-py3
- a modified copy of the BU's HotSpot 3D-IC thermal analyzer; the code should be provided
along with Corblivar. Note that this code has to be compiled separately.

To use Corblivar, the following procedure should be followed
============================================================

1) Configuration of HotSpot: see ../HotSpot/hotspot.config
----------------------------------------------------------

Relevant are the specs for the heat sink and heat spreader; they should be adapted to
reflect largest chip dimensions under consideration.

2) Configuration of Corblivar: see exp/Corblivar.conf or other examples in exp/configs
--------------------------------------------------------------------------------------

Most relevant is the section "3D-IC parameters", i.e., the number of layers in the IC
stack and the fixed, common outline of these layers.

The section "SA -- Layout generation options" can be used to configure the optimization
heuristic. Reasonable values depend on the experiments under consideration. Some options,
like power-aware block handling, are to be considered carefully. The latter acts as a
hard constraint such that blocks with high power consumption are placed in the upper
layer, nearest to the heatsink. Note that this has the largest positive impact on thermal
management, i.e., notably greater impact than thermal-aware placement within separate
layers. This option may, however, render some specific block-alignment configurations
infeasible. For example, two high-power blocks could then not be aligned across layers
for embedding of vertical buses.

The sections "SA -- Loop parameters" and "SA -- Temperature schedule parameters" control
the runtime behaviour of the optimization. These values can impact the success rate,
especially for ``tight'' configurations with many block-alignment requests and/or dense
packing. The latter section, however, is considered to be applicable for different
experiments---the adaptive SA-optimization schedule draws the cooling parameters somewhat
non-relevant since local minima in the solution space can be escaped easily by iterative
temperature increases.

The section "SA -- Factors for second-phase cost function" controls the various
optimization modules; the related values should be adapted to reflect the desired
optimization cost function. Note that zero values deactivates the respective optimization
completely and thus allows to save some runtime.

The section "Power blurring (thermal analysis) -- Default thermal-mask parameters" can be
left as is; the related parametrization is done via separate scripts, as described in the
next step.

3) Parametrization of power blurring: see thermal_analysis_fitting/ and documentation_Octave.pdf
------------------------------------------------------------------------------------------------

As indicated in 2), the thermal-mask parameters are obtained separately. The related
Octave scripts should be run whenever the 3D-IC setup changes notably, i.e., when the
number of layers, the outline, the heatsink, and/or the (magnitude of) power consumption
of the benchmarks changes.

To configure the Octave scripts, see thermal_analysis_fitting/parameters.m

To run the Octave scripts, either change directory to thermal_analysis_fitting/ and start
scripts from there (octave optimization.m BENCH CORBLIVAR.CONF), or copy the scripts from
thermal_analysis_fitting/ to separate working directories; see below and/or exp/run9.sh
for further details.

The Octave scripts work like this: first, generate a floorplan solution; second, run
HotSpot on this solution (note that specific and heterogeneous TSV densities are considered here);
third, match the power-blurring temperature map to the HotSpot map via a local search;
fourth, output the related power-blurring parameters for the best match, which describes
the HotSpot estimate most closely. For further details, see documentation_Octave.pdf.

Note that Corblivar models the thermal impact of both regular signal TSVs and vertical
buses, i.e., large TSV groups. Currently, regular signal TSVs are assumed placable within
their related nets' bounding boxes, i.e., their superposed TSV densities are accordingly
spread across the whole 3D-IC. Also, vertical buses are assumed to have tightest possible
packing of multiple TSVs (100% TSV density) for the whole bus region, even if fewer TSVs
would suffice for signal transmission.

4) Actual run of Corblivar: see exp/run*.sh or directly start ./Corblivar
-------------------------------------------------------------------------

To run Corblivar, one can start the binary directly, for example from the exp/ folder as
../Corblivar BENCH CORBLIVAR.CONF benches/
The other option is to call Corblivar in a batch mode, as outlined in the scripts
exp/run*.sh

Note that for generation of plotted data, one has to call the script exp/gp.sh afterwards
in the related working directory.

The further comments below are for understanding of the Corblivar tool and its structure
========================================================================================

Various experiments can be started using exp/run*.sh; these scripts are not a complete
set for running all experiments but rather a guideline for different setups.

The folder exp/benches/ includes MCNC (some are not working, i.e., have issues with their
content), GSRC, and IBM-HB+ benchmarks, all in the GSRC format

The folder thermal_analysis_fitting/ includes Octave scripts for the parameterization of
the power-blurring-based thermal analysis; they can be also included e.g. in run*.sh
scripts.  Note that these scripts will produce temporary output data in
thermal_analysis_fitting/, i.e., you might want to copy thermal_analysis_fitting/ into
separate working directories for parallel execution of different experiments, to avoid
mixed up data. See for example exp/run9.sh

The script exp/gp.sh delegates to gnuplot for generating various output plots, e.g.,
thermal map and floorplan, after running Corbilvar.

The script HotSpot.sh calls a (slightly modified) version of BU's 3D HotSpot program; the
related code should be provided along with Corblivar.

The file exp/Corblivar.conf is a template for the config file required by Corblivar,
further examples can be found in exp/configs/.

Settings in some scripts may rely on the folder ~/code ; putting ~/code/Corblivar along
with ~/code/HotSpot is the default setup which should be working in any case.

Changelog
=========

1.1.1 (May 7, 2014, commit 58e43811eed5466b505fa0a16d4778793f9e1e8e): updates, consideration of heterogeneous TSV densities
---------------------------------------------------------------------------------------------------------------------------
- dropped deprecated handling of different masks
- dropped dummy TSV handling
- added handler for TSV densities considering both signal TSVs and vertical buses
- Octave script now considers parameter for scaling down power in TSV regions
- various minor updates and fixes
1.1.0 (Nov 13, 2013, commit 743598a2835826d941f3a4f16ce64df3938996a5): new feature, consideration of heterogeneous TSV densities
--------------------------------------------------------------------------------------------------------------------------------
- adapted power blurring for using different masks
- added plotting of TSV-density maps
- adapted HotSpot file handler
- added dummy TSV handler
- Octave script now considers determination of different masks
- various minor updates and fixes
1.0.4 (Aug 21, 2013, commit e2c4890f03333a9849308f0649eb09b290982e06): fixes and updates, thermal analysis
----------------------------------------------------------------------------------------------------------
1.0.3 (Aug 1, 2013, commit 76ed57a49082f678edc65102f0a25c1b3ffc6991): fix, compiling error for 64-bit libaries
--------------------------------------------------------------------------------------------------------------
1.0.2 (Jul 29, 2013, commit bcbb91f21029207f430bc68e52b8c1cb3083df7a): update, enable fixed-position block alignment
--------------------------------------------------------------------------------------------------------------------
1.0.1 (Jul 29, 2013, commit 06a6844e48e4a0a066b8483e443d49c949f5fc26): bugfixes and updates
-------------------------------------------------------------------------------------------
- update HotSpot BU to v 1.2
- fixes calculation of thermal-related material properties
- new class Chip contains all chip-related settings
1.0.0 (Jul 22, 2013, commit 8eb78f7d17dc6ce25458a629072e900bfa31a22a): initial public release
---------------------------------------------------------------------------------------------
