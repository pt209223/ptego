/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 *                                                                           *
 *  This file is part of Library of Effective GO routines - EGO library      *
 *                                                                           *
 *  Copyright 2006 and onwards, Lukasz Lew                                   *
 *                                                                           *
 *  EGO library is free software; you can redistribute it and/or modify      *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation; either version 2 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  EGO library is distributed in the hope that it will be useful,           *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with EGO library; if not, write to the Free Software               *
 *  Foundation, Inc., 51 Franklin St, Fifth Floor,                           *
 *  Boston, MA  02110-1301  USA                                              *
 *                                                                           *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "playout.h"

const Vertex ExtPolicy::begin1[25] = {
	Vertex(3,3),Vertex(3,4),Vertex(3,5),Vertex(3,6),Vertex(3,7),
	Vertex(4,3),Vertex(4,4),Vertex(4,5),Vertex(4,6),Vertex(4,7),
	Vertex(5,3),Vertex(5,4),Vertex(5,5),Vertex(5,6),Vertex(5,7),
	Vertex(6,3),Vertex(6,4),Vertex(6,5),Vertex(6,6),Vertex(6,7),
	Vertex(7,3),Vertex(7,4),Vertex(7,5),Vertex(7,6),Vertex(7,7)
};

const Vertex ExtPolicy::begin2[24] = {
	Vertex(2,2),Vertex(2,3),Vertex(2,4),Vertex(2,5),Vertex(2,6),Vertex(2,7),Vertex(2,8),
	Vertex(3,2),                                                            Vertex(3,8),
	Vertex(4,2),                                                            Vertex(4,8),
	Vertex(5,2),                                                            Vertex(5,8),
	Vertex(6,2),                                                            Vertex(6,8),
	Vertex(7,2),                                                            Vertex(7,8),
	Vertex(8,2),Vertex(8,3),Vertex(8,4),Vertex(8,5),Vertex(8,6),Vertex(8,7),Vertex(8,8)
};
