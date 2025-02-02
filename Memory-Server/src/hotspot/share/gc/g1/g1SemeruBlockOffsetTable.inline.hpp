/*
 * Copyright (c) 2001, 2017, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#ifndef SHARE_VM_GC_G1_SEMERU_G1BLOCKOFFSETTABLE_INLINE_HPP
#define SHARE_VM_GC_G1_SEMERU_G1BLOCKOFFSETTABLE_INLINE_HPP

#include "gc/g1/g1SemeruBlockOffsetTable.hpp"
#include "gc/g1/SemeruHeapRegion.hpp"
#include "gc/shared/memset_with_concurrent_readers.hpp"
#include "gc/shared/space.hpp"

inline HeapWord* G1SemeruBlockOffsetTablePart::block_start(const void* addr) {
  if (addr >= _space->bottom() && addr < _space->end()) {
    HeapWord* q = block_at_or_preceding(addr, true, _next_offset_index-1);   // [?] q is the start addr of the first oop of the card containing addr.
    return forward_to_block_containing_addr(q, addr);
  } else {
    return NULL;
  }
}

inline HeapWord* G1SemeruBlockOffsetTablePart::block_start_const(const void* addr) const {
  if (addr >= _space->bottom() && addr < _space->end()) {
    HeapWord* q = block_at_or_preceding(addr, true, _next_offset_index-1);
    HeapWord* n = q + block_size(q);
    return forward_to_block_containing_addr_const(q, n, addr);
  } else {
    return NULL;
  }
}

u_char G1SemeruBlockOffsetTable::offset_array(size_t index) const {
  check_index(index, "index out of range");
  return _offset_array[index];
}

void G1SemeruBlockOffsetTable::set_offset_array(size_t index, u_char offset) {
  check_index(index, "index out of range");
  set_offset_array_raw(index, offset);
}

void G1SemeruBlockOffsetTable::set_offset_array(size_t index, HeapWord* high, HeapWord* low) {
  check_index(index, "index out of range");
  assert(high >= low, "addresses out of order");
  size_t offset = pointer_delta(high, low);
  check_offset(offset, "offset too large");
  set_offset_array(index, (u_char)offset);
}

void G1SemeruBlockOffsetTable::set_offset_array(size_t left, size_t right, u_char offset) {
  check_index(right, "right index out of range");
  assert(left <= right, "indexes out of order");
  size_t num_cards = right - left + 1;
  memset_with_concurrent_readers(&_offset_array[left], offset, num_cards);
}

// Variant of index_for that does not check the index for validity.
// BOTConstants::LogN == 9, card size 512 bytes
//
inline size_t G1SemeruBlockOffsetTable::index_for_raw(const void* p) const {
  return pointer_delta((char*)p, _reserved.start(), sizeof(char)) >> BOTConstants::LogN;
}

// Calculate the Block index for the address p.
// For the covered space, stored in _reserved. index start from 0.
inline size_t G1SemeruBlockOffsetTable::index_for(const void* p) const {
  char* pc = (char*)p;
  assert(pc >= (char*)_reserved.start() &&
         pc <  (char*)_reserved.end(),
         "p (" PTR_FORMAT ") not in reserved [" PTR_FORMAT ", " PTR_FORMAT ")",
         p2i(p), p2i(_reserved.start()), p2i(_reserved.end()));
  size_t result = index_for_raw(p);
  check_index(result, "bad index from address");
  return result;
}

inline HeapWord* G1SemeruBlockOffsetTable::address_for_index(size_t index) const {
  check_index(index, "index out of range");
  HeapWord* result = address_for_index_raw(index);
  assert(result >= _reserved.start() && result < _reserved.end(),
         "bad address from index result " PTR_FORMAT
         " _reserved.start() " PTR_FORMAT " _reserved.end() " PTR_FORMAT,
         p2i(result), p2i(_reserved.start()), p2i(_reserved.end()));
  return result;
}

inline size_t G1SemeruBlockOffsetTablePart::block_size(const HeapWord* p) const {
  return _space->block_size(p);  // [?] What's the purpose of this block size ? not 512 bytes ??
}

inline HeapWord* G1SemeruBlockOffsetTablePart::block_at_or_preceding(const void* addr,
                                                               bool has_max_index,
                                                               size_t max_index) const {
  assert(_object_can_span || _bot->offset_array(_bot->index_for(_space->bottom())) == 0,
         "Object crossed region boundary, found offset %u instead of 0",
         (uint) _bot->offset_array(_bot->index_for(_space->bottom())));
  size_t index = _bot->index_for(addr);
  // We must make sure that the offset table entry we use is valid.  If
  // "addr" is past the end, start at the last known one and go forward.
  if (has_max_index) {
    index = MIN2(index, max_index);
  }
  HeapWord* q = _bot->address_for_index(index);  // start addr for the block index.

  uint offset = _bot->offset_array(index);  // Extend u_char to uint.
  while (offset >= BOTConstants::N_words) {
    // The excess of the offset from N_words indicates a power of Base
    // to go back by.
    size_t n_cards_back = BOTConstants::entry_to_cards_back(offset);
    q -= (BOTConstants::N_words * n_cards_back);
    index -= n_cards_back;
    offset = _bot->offset_array(index);
  }
  assert(offset < BOTConstants::N_words, "offset too large");
  q -= offset;  //[x] let p points to the first object in this card.
  return q;
}

inline HeapWord* G1SemeruBlockOffsetTablePart::forward_to_block_containing_addr_const(HeapWord* q, HeapWord* n,
                                                                                const void* addr) const {
  if (addr >= _space->top()) return _space->top();
  while (n <= addr) {
    q = n;
    oop obj = oop(q);
    if (obj->klass_or_null_acquire() == NULL) {
      return q;
    }
    n += block_size(q);
  }
  assert(q <= n, "wrong order for q and addr");
  assert(addr < n, "wrong order for addr and n");
  return q;
}

inline HeapWord* G1SemeruBlockOffsetTablePart::forward_to_block_containing_addr(HeapWord* q,
                                                                          const void* addr) {
  if (oop(q)->klass_or_null_acquire() == NULL) {
    return q;
  }
  HeapWord* n = q + block_size(q);
  // In the normal case, where the query "addr" is a card boundary, and the
  // offset table chunks are the same size as cards, the block starting at
  // "q" will contain addr, so the test below will fail, and we'll fall
  // through quickly.
  if (n <= addr) {
    q = forward_to_block_containing_addr_slow(q, n, addr);
  }
  assert(q <= addr, "wrong order for current and arg");
  return q;
}



#endif // SHARE_VM_GC_G1_G1BLOCKOFFSETTABLE_INLINE_HPP
