/*
 * =====================================================================================
 *
 *    Description:  Corblivar layout operations
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

// own Corblivar header
#include "LayoutOperations.hpp"
// required Corblivar headers
#include "Math.hpp"
#include "CorblivarCore.hpp"
#include "CorblivarAlignmentReq.hpp"

// memory allocation
constexpr int LayoutOperations::OP_SWAP_BLOCKS;
constexpr int LayoutOperations::OP_MOVE_TUPLE;

bool LayoutOperations::performRandomLayoutOp(CorblivarCore& corb, bool const& SA_phase_two, bool const& revertLastOp) {
	int op;
	int die1, die2, tuple1, tuple2, juncts;
	bool ret, swapping_failed_blocks;

	if (LayoutOperations::DBG) {
		cout << "-> LayoutOperations::performRandomLayoutOp(" << &corb << ", " << SA_phase_two << ", " << revertLastOp << ")" << endl;
	}

	// init layout operation variables
	die1 = die2 = tuple1 = tuple2 = juncts = -1;

	// revert last op
	if (revertLastOp) {
		op = this->last_op;
	}
	// perform new op
	else {

		// to enable guided block alignment during phase II, we prefer to perform
		// block swapping on particular blocks of failing alignment requests
		swapping_failed_blocks = false;
		if (SA_phase_two && this->parameters.opt_alignment) {

			// try to setup swapping failed blocks
			swapping_failed_blocks = this->prepareBlockSwappingFailedAlignment(corb, die1, tuple1, die2, tuple2);
			this->last_op = op = LayoutOperations::OP_SWAP_BLOCKS;
		}

		// for other regular cases or in case swapping failed blocks was not successful, we proceed with a random
		// operation
		if (!swapping_failed_blocks) {

			// reset layout operation variables
			die1 = die2 = tuple1 = tuple2 = juncts = -1;

			// see defined op-codes to set random-number ranges; recall that
			// randI(x,y) is [x,y)
			this->last_op = op = Math::randI(1, 6);
		}
	}

	// specific op handler
	switch (op) {

		case LayoutOperations::OP_SWAP_BLOCKS: // op-code: 1

			ret = this->performOpMoveOrSwapBlocks(LayoutOperations::OP_SWAP_BLOCKS, revertLastOp, !SA_phase_two, corb, die1, die2, tuple1, tuple2);

			break;

		case LayoutOperations::OP_MOVE_TUPLE: // op-code: 2

			ret = this->performOpMoveOrSwapBlocks(LayoutOperations::OP_MOVE_TUPLE, revertLastOp, !SA_phase_two, corb, die1, die2, tuple1, tuple2);

			break;

		case LayoutOperations::OP_SWITCH_INSERTION_DIR: // op-code: 3

			ret = this->performOpSwitchInsertionDirection(revertLastOp, corb, die1, tuple1);

			break;

		case LayoutOperations::OP_SWITCH_TUPLE_JUNCTS: // op-code: 4

			ret = this->performOpSwitchTupleJunctions(revertLastOp, corb, die1, tuple1, juncts);

			break;

		case LayoutOperations::OP_ROTATE_BLOCK__SHAPE_BLOCK: // op-code: 5

			ret = this->performOpShapeBlock(revertLastOp, corb, die1, tuple1);

			break;
	}

	// memorize elements of successful op
	if (ret) {
		this->last_op_die1 = die1;
		this->last_op_die2 = die2;
		this->last_op_tuple1 = tuple1;
		this->last_op_tuple2 = tuple2;
		this->last_op_juncts = juncts;
	}

	if (LayoutOperations::DBG) {
		cout << "<- LayoutOperations::performRandomLayoutOp : " << ret << endl;
	}

	return ret;
}

bool LayoutOperations::prepareBlockSwappingFailedAlignment(CorblivarCore const& corb, int& die1, int& tuple1, int& die2, int& tuple2) {
	CorblivarAlignmentReq const* failed_req = nullptr;
	Block const* b1;
	Block const* b1_neighbour = nullptr;

	// determine first failed alignment
	for (unsigned r = 0; r < corb.getAlignments().size(); r++) {

		if (!corb.getAlignments()[r].fulfilled) {
			failed_req = &corb.getAlignments()[r];
			break;
		}
	}

	// handle request; sanity check for found failed request
	if (failed_req != nullptr) {

		// randomly decide for one block; consider the dummy reference block if
		// required
		if (
			// randomly select s_i if it's not the RBOD
			(failed_req->s_i->id != "RBOD" && Math::randB()) ||
			// if s_j is the RBOD, we need to use s_i; assuming that
			// only s_i OR s_j are the RBOD
			failed_req->s_j->id == "RBOD"
		   ) {
			die1 = die2 = failed_req->s_i->layer;
			tuple1 = corb.getDie(die1).getTuple(failed_req->s_i);
			b1 = failed_req->s_i;
		}
		else {
			die1 = die2 = failed_req->s_j->layer;
			tuple1 = corb.getDie(die1).getTuple(failed_req->s_j);
			b1 = failed_req->s_j;
		}

		if (
			// zero-offset fixed alignment in both coordinates
			(failed_req->offset_x() && failed_req->alignment_x == 0 && failed_req->offset_y() && failed_req->alignment_y == 0) ||
			// min overlap in both dimensions
			(failed_req->range_x() && failed_req->range_y())
		   ) {

			// such alignment cannot be fulfilled in one die, i.e., differing
			// dies required
			if (failed_req->s_i->layer == failed_req->s_j->layer) {

				// this is only possible for > 1 layers; sanity check
				if (this->parameters.layers == 1) {
					return false;
				}

				while (die1 == die2) {
					die2 = Math::randI(0, this->parameters.layers);
				}
			}

			// select block to swap with such that blocks to be aligned are
			// initially at least intersecting blocks
			for (Block const* b2 : corb.getDie(die2).getBlocks()) {

				if (Rect::rectsIntersect(b1->bb, b2->bb)) {

					// however, this should not be the partner block
					// of the alignment request
					if (
						(b1->id == failed_req->s_i->id && b2->id == failed_req->s_j->id) ||
						(b1->id == failed_req->s_j->id && b2->id == failed_req->s_i->id)
					   ) {
						continue;
					}

					b1_neighbour = b2;

					break;
				}
			}
		}

		// failed alignment ranges and (partially) non-zero-offset fixed alignment
		//
		// determine relevant neighbour block to perform swap operation, i.e.,
		// nearest neighbour w.r.t. failure type
		else {

			// also consider randomly to change die2 as well; this is required
			// for alignments which cannot be fulfilled within one die and
			// does not harm for alignments which could be fulfilled within
			// one die (they can then also be fulfilled across dies); note
			// that an explicit check for all the different options of
			// alignments not possible within one die are not performed here
			// but rather a die change is considered randomly
			//
			// note that changing dies is only possible for > 1 layers
			if (Math::randB() && this->parameters.layers > 1) {

				while (die1 == die2) {
					die2 = Math::randI(0, this->parameters.layers);
				}
			}
		
			for (Block const* b2 : corb.getDie(die2).getBlocks()) {

				switch (b1->alignment) {

					// determine nearest right block
					case Block::AlignmentStatus::FAIL_HOR_TOO_LEFT:

						if (Rect::rectA_leftOf_rectB(b1->bb, b2->bb, true)) {

							if (b1_neighbour == nullptr || b2->bb.ll.x < b1_neighbour->bb.ll.x) {
								b1_neighbour = b2;
							}
						}

						break;

					// determine nearest left block
					case Block::AlignmentStatus::FAIL_HOR_TOO_RIGHT:

						if (Rect::rectA_leftOf_rectB(b2->bb, b1->bb, true)) {

							if (b1_neighbour == nullptr || b2->bb.ur.x > b1_neighbour->bb.ur.x) {
								b1_neighbour = b2;
							}
						}

						break;

					// determine nearest block above
					case Block::AlignmentStatus::FAIL_VERT_TOO_LOW:

						if (Rect::rectA_below_rectB(b1->bb, b2->bb, true)) {

							if (b1_neighbour == nullptr || b2->bb.ll.y < b1_neighbour->bb.ll.y) {
								b1_neighbour = b2;
							}
						}

						break;

					// determine nearest block below
					case Block::AlignmentStatus::FAIL_VERT_TOO_HIGH:

						if (Rect::rectA_below_rectB(b2->bb, b1->bb, true)) {

							if (b1_neighbour == nullptr || b2->bb.ur.y > b1_neighbour->bb.ur.y) {
								b1_neighbour = b2;
							}
						}

						break;

					// dummy case, to catch other (here not occuring)
					// alignment status
					default:
						break;
				}
			}
		}

		// determine related tuple of neigbhour block; == -1 in case the tuple
		// cannot be find; sanity check for undefined neighbour
		if (b1_neighbour != nullptr) {

			tuple2 = corb.getDie(die2).getTuple(b1_neighbour);

			if (CorblivarAlignmentReq::DBG) {
				cout << "DBG_ALIGNMENT> " << failed_req->tupleString() << " failed so far;" << endl;
				cout << "DBG_ALIGNMENT> considering swapping block " << b1->id << " on layer " << b1->layer;
				cout << " with block " << b1_neighbour->id << " on layer " << b1_neighbour->layer << endl;
			}

			return true;
		}
		else {
			tuple2 = -1;

			return false;
		}
	}
	else {
		return false;
	}
}

bool LayoutOperations::performOpEnhancedSoftBlockShaping(CorblivarCore const& corb, Block const* shape_block) const {
	int op;
	double boundary_x, boundary_y;
	double width, height;

	// see defined op-codes in class FloorPlanner to set random-number ranges;
	// recall that randI(x,y) is [x,y)
	op = Math::randI(10, 15);

	switch (op) {

		// stretch such that shape_block's right front aligns w/ the right front
		// of the nearest other block
		case LayoutOperations::OP_SHAPE_BLOCK__STRETCH_HORIZONTAL: // op-code: 10

			// dummy value, to be large than right front
			boundary_x = 2.0 * shape_block->bb.ur.x;

			for (Block const* b : corb.getDie(shape_block->layer).getBlocks()) {

				// determine nearest right front of other blocks
				if (b->bb.ur.x > shape_block->bb.ur.x) {
					boundary_x = min(boundary_x, b->bb.ur.x);
				}
			}

			// determine resulting new dimensions
			width = boundary_x - shape_block->bb.ll.x;
			height = shape_block->bb.area / width;

			// apply new dimensions in case the resulting AR is allowed
			return shape_block->shapeByWidthHeight(width, height);

		// shrink such that shape_block's right front aligns w/ the left front of
		// the nearest other block
		case LayoutOperations::OP_SHAPE_BLOCK__SHRINK_HORIZONTAL: // op-code: 12

			boundary_x = 0.0;

			for (Block const* b : corb.getDie(shape_block->layer).getBlocks()) {

				// determine nearest left front of other blocks
				if (b->bb.ll.x < shape_block->bb.ur.x) {
					boundary_x = max(boundary_x, b->bb.ll.x);
				}
			}

			// determine resulting new dimensions
			width = boundary_x - shape_block->bb.ll.x;
			height = shape_block->bb.area / width;

			// apply new dimensions in case the resulting AR is allowed
			return shape_block->shapeByWidthHeight(width, height);

		// stretch such that shape_block's top front aligns w/ the top front of
		// the nearest other block
		case LayoutOperations::OP_SHAPE_BLOCK__STRETCH_VERTICAL: // op-code: 11

			// dummy value, to be large than top front
			boundary_y = 2.0 * shape_block->bb.ur.y;

			for (Block const* b : corb.getDie(shape_block->layer).getBlocks()) {

				// determine nearest top front of other blocks
				if (b->bb.ur.y > shape_block->bb.ur.y) {
					boundary_y = min(boundary_y, b->bb.ur.y);
				}
			}

			// determine resulting new dimensions
			height = boundary_y - shape_block->bb.ll.y;
			width = shape_block->bb.area / height;

			// apply new dimensions in case the resulting AR is allowed
			return shape_block->shapeByWidthHeight(width, height);

		// shrink such that shape_block's top front aligns w/ the bottom front of
		// the nearest other block
		case LayoutOperations::OP_SHAPE_BLOCK__SHRINK_VERTICAL: // op-code: 13

			boundary_y = 0.0;

			for (Block const* b : corb.getDie(shape_block->layer).getBlocks()) {

				// determine nearest bottom front of other blocks
				if (b->bb.ll.y < shape_block->bb.ur.y) {
					boundary_y = max(boundary_y, b->bb.ll.y);
				}
			}

			// determine resulting new dimensions
			height = boundary_y - shape_block->bb.ll.y;
			width = shape_block->bb.area / height;

			// apply new dimensions in case the resulting AR is allowed
			return shape_block->shapeByWidthHeight(width, height);

		case LayoutOperations::OP_SHAPE_BLOCK__RANDOM_AR: // op-code: 14

			shape_block->shapeRandomlyByAR();

			return true;

		// to avoid compiler warnings, non-reachable code due to
		// constrained op value
		default:
			return false;
	}
}

bool LayoutOperations::performOpEnhancedHardBlockRotation(CorblivarCore const& corb, Block const* shape_block) const {
	double col_max_width, row_max_height;
	double gain, loss;

	// horizontal block
	if (shape_block->bb.w > shape_block->bb.h) {

		// check blocks in (implicitly constructed) row
		row_max_height = shape_block->bb.h;

		for (Block const* b : corb.getDie(shape_block->layer).getBlocks()) {

			if (shape_block->bb.ll.y == b->bb.ll.y) {
				row_max_height = max(row_max_height, b->bb.h);
			}
		}

		// gain in horizontal direction by rotation
		gain = shape_block->bb.w - shape_block->bb.h;
		// loss in vertical direction; only if new block
		// height (current width) would be larger than the
		// row's current height
		loss = shape_block->bb.w - row_max_height;
	}
	// vertical block
	else {
		// check blocks in (implicitly constructed) column
		col_max_width = shape_block->bb.w;

		for (Block const* b : corb.getDie(shape_block->layer).getBlocks()) {

			if (shape_block->bb.ll.x == b->bb.ll.x) {
				col_max_width = max(col_max_width, b->bb.w);
			}
		}

		// gain in vertical direction by rotation
		gain = shape_block->bb.h - shape_block->bb.w;
		// loss in horizontal direction; only if new block
		// width (current height) would be larger than the
		// column's current width
		loss = shape_block->bb.h - col_max_width;
	}

	// perform rotation if no loss or gain > loss
	if (loss < 0.0 || gain > loss) {
		shape_block->rotate();

		return true;
	}
	else {
		return false;
	}
}


bool LayoutOperations::performOpSwitchTupleJunctions(bool const& revert, CorblivarCore& corb, int& die1, int& tuple1, int& juncts) const {
	int new_juncts;

	if (!revert) {

		// randomly select die, if not preassigned
		if (die1 == -1) {
			die1 = Math::randI(0, this->parameters.layers);
		}

		// sanity check for empty dies
		if (corb.getDie(die1).getCBL().empty()) {
			return false;
		}

		// randomly select tuple, if not preassigned
		if (tuple1 == -1) {
			tuple1 = Math::randI(0, corb.getDie(die1).getCBL().size());
		}

		// juncts is for return-by-reference, new_juncts for updating junctions
		new_juncts = juncts = corb.getDie(die1).getJunctions(tuple1);

		// junctions must be geq 0
		if (new_juncts == 0) {
			new_juncts++;
		}
		else {
			if (Math::randB()) {
				new_juncts++;
			}
			else {
				new_juncts--;
			}
		}

		corb.switchTupleJunctions(die1, tuple1, new_juncts);
	}
	else {
		corb.switchTupleJunctions(this->last_op_die1, this->last_op_tuple1, this->last_op_juncts);
	}

	return true;
}

bool LayoutOperations::performOpSwitchInsertionDirection(bool const& revert, CorblivarCore& corb, int& die1, int& tuple1) const {
	if (!revert) {

		// randomly select die, if not preassigned
		if (die1 == -1) {
			die1 = Math::randI(0, this->parameters.layers);
		}

		// sanity check for empty dies
		if (corb.getDie(die1).getCBL().empty()) {
			return false;
		}

		// randomly select tuple, if not preassigned
		if (tuple1 == -1) {
			tuple1 = Math::randI(0, corb.getDie(die1).getCBL().size());
		}

		corb.switchInsertionDirection(die1, tuple1);
	}
	else {
		corb.switchInsertionDirection(this->last_op_die1, this->last_op_tuple1);
	}

	return true;
}

bool LayoutOperations::performOpMoveOrSwapBlocks(int const& mode, bool const& revert, bool const& SA_phase_one, CorblivarCore& corb, int& die1, int& die2, int& tuple1, int& tuple2) const {

	if (!revert) {

		// randomly select die, if not preassigned
		if (die1 == -1) {
			die1 = Math::randI(0, this->parameters.layers);
		}
		if (die2 == -1) {
			die2 = Math::randI(0, this->parameters.layers);
		}

		if (mode == LayoutOperations::OP_MOVE_TUPLE) {
			// sanity check for empty (origin) die
			if (corb.getDie(die1).getCBL().empty()) {
				return false;
			}
		}
		else if (mode == LayoutOperations::OP_SWAP_BLOCKS) {
			// sanity check for empty dies
			if (corb.getDie(die1).getCBL().empty() || corb.getDie(die2).getCBL().empty()) {
				return false;
			}
		}

		// randomly select tuple, if not preassigned
		if (tuple1 == -1) {
			tuple1 = Math::randI(0, corb.getDie(die1).getCBL().size());
		}
		if (tuple2 == -1) {
			tuple2 = Math::randI(0, corb.getDie(die2).getCBL().size());
		}

		// in case of swapping/moving w/in same die, ensure that tuples are
		// different
		if (die1 == die2) {
			// this is, however, only possible if at least two
			// tuples are given in that die
			if (corb.getDie(die1).getCBL().size() < 2) {
				return false;
			}
			// determine two different tuples
			while (tuple1 == tuple2) {
				tuple2 = Math::randI(0, corb.getDie(die1).getCBL().size());
			}
		}

		// for power-aware block handling, ensure that blocks w/ lower power
		// density remain in lower layer
		if (this->parameters.power_aware_block_handling) {
			if (die1 < die2
					&& (corb.getDie(die1).getBlock(tuple1)->power_density < corb.getDie(die2).getBlock(tuple2)->power_density)) {
				return false;
			}
			else if (die2 < die1
					&& (corb.getDie(die2).getBlock(tuple2)->power_density < corb.getDie(die1).getBlock(tuple1)->power_density)) {
				return false;
			}
		}

		// for SA phase one, floorplacement blocks, i.e., large macros, should not
		// be moved/swapped
		if (this->parameters.floorplacement && SA_phase_one
				&& (corb.getDie(die1).getBlock(tuple1)->floorplacement || corb.getDie(die2).getBlock(tuple2)->floorplacement)) {
			return false;
		}

		// perform move/swap; applies only to valid candidates
		if (mode == LayoutOperations::OP_MOVE_TUPLE) {
			corb.moveTuples(die1, die2, tuple1, tuple2);
		}
		else if (mode == LayoutOperations::OP_SWAP_BLOCKS) {
			corb.swapBlocks(die1, die2, tuple1, tuple2);
		}
	}
	else {
		if (mode == LayoutOperations::OP_MOVE_TUPLE) {
			corb.moveTuples(this->last_op_die2, this->last_op_die1, this->last_op_tuple2, this->last_op_tuple1);
		}
		else if (mode == LayoutOperations::OP_SWAP_BLOCKS) {
			corb.swapBlocks(this->last_op_die1, this->last_op_die2, this->last_op_tuple1, this->last_op_tuple2);
		}
	}

	return true;
}

bool LayoutOperations::performOpShapeBlock(bool const& revert, CorblivarCore& corb, int& die1, int& tuple1) const {
	Block const* shape_block;

	if (!revert) {

		// randomly select die, if not preassigned
		if (die1 == -1) {
			die1 = Math::randI(0, this->parameters.layers);
		}

		// sanity check for empty dies
		if (corb.getDie(die1).getCBL().empty()) {
			return false;
		}

		// randomly select tuple, if not preassigned
		if (tuple1 == -1) {
			tuple1 = Math::randI(0, corb.getDie(die1).getCBL().size());
		}

		// determine related block to be shaped
		shape_block = corb.getDie(die1).getBlock(tuple1);

		// backup current shape
		shape_block->bb_backup = shape_block->bb;

		// soft blocks: enhanced block shaping
		if (shape_block->soft) {
			// enhanced shaping, according to [Chen06]
			if (this->parameters.enhanced_soft_block_shaping) {
				return this->performOpEnhancedSoftBlockShaping(corb, shape_block);
			}
			// simple random shaping
			else {
				shape_block->shapeRandomlyByAR();
			}
		}
		// hard blocks: simple rotation or enhanced rotation (perform block
		// rotation only if layout compaction is achievable); note that this
		// enhanced rotation relies on non-compacted, i.e., non-packed layouts,
		// which is checked during config file parsing
		else {
			// enhanced rotation
			if (this->parameters.enhanced_hard_block_rotation) {
				return this->performOpEnhancedHardBlockRotation(corb, shape_block);
			}
			// simple rotation
			else {
				shape_block->rotate();
			}
		}
	}
	// revert last rotation
	else {
		// revert by restoring backup bb
		corb.getDie(this->last_op_die1).getBlock(this->last_op_tuple1)->bb =
			corb.getDie(this->last_op_die1).getBlock(this->last_op_tuple1)->bb_backup;
	}

	return true;
}