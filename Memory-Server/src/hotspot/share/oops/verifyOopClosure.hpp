/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_VM_OOPS_VERIFYOOPCLOSURE_HPP
#define SHARE_VM_OOPS_VERIFYOOPCLOSURE_HPP

#include "memory/iterator.hpp"

class VerifyOopClosure: public OopClosure {
 protected:
  template <class T> void do_oop_work(T* p);
 public:
  virtual void do_oop(oop* p);
  virtual void do_oop(narrowOop* p);

  virtual inline void semeru_ms_do_oop(oop obj, oop* p) { do_oop(p);  }
  virtual inline void semeru_ms_do_oop(oop obj, narrowOop* p) { do_oop(p);  }

  static VerifyOopClosure verify_oop;
};

#endif // SHARE_VM_OOPS_VERIFYOOPCLOSURE_HPP
