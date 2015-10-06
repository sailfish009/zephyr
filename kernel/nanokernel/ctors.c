/* ctors.c - constructor module */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
DESCRIPTION
This module provides the C++ style constructor mechanism used by various
components to initialize themselves automatically.

The _Ctors() routine is called from the nanokernel's _Cstart() routine after
hardware initialization has completed.

Although ctors are traditionally a C++ feature, normal C code can use them too
through the appropriate use of GCC's constructor attribute.
No destructor support (dtors) is provided.
 */

/* What a constructor function pointer looks like */

typedef void (*CtorFuncPtr)(void);

/* Constructor function pointer list is generated by the linker script. */

extern CtorFuncPtr __CTOR_LIST__[];
extern CtorFuncPtr __CTOR_END__[];

/**
 *
 * @brief Invoke all C++ style global object constructors
 *
 * This routine is invoked by the nanokernel routine _Cstart() after the basic
 * hardware has been initialized.
 */

void _Ctors(void)
{
	unsigned int nCtors;

	nCtors = (unsigned int)__CTOR_LIST__[0];

	while (nCtors >= 1) {
		__CTOR_LIST__[nCtors--]();
	}
}
