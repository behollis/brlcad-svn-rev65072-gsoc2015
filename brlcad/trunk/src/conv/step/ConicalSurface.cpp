/*                 ConicalSurface.cpp
 * BRL-CAD
 *
 * Copyright (c) 1994-2009 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file ConicalSurface.cpp
 *
 * Routines to interface to STEP "ConicalSurface".
 *
 */
#include "STEPWrapper.h"
#include "Factory.h"

#include "Axis2Placement3D.h"

#include "ConicalSurface.h"

#define CLASSNAME "ConicalSurface"
#define ENTITYNAME "Conical_Surface"
string ConicalSurface::entityname = Factory::RegisterClass(ENTITYNAME,(FactoryMethod)ConicalSurface::Create);

ConicalSurface::ConicalSurface() {
	step = NULL;
	id = 0;
}

ConicalSurface::ConicalSurface(STEPWrapper *sw,int STEPid) {
	step=sw;
	id = STEPid;
}

ConicalSurface::~ConicalSurface() {
}

const double *
ConicalSurface::GetOrigin() {
	return position->GetOrigin();
}

const double *
ConicalSurface::GetNormal() {
	return position->GetAxis(2);
}

const double *
ConicalSurface::GetXAxis() {
	return position->GetXAxis();
}

const double *
ConicalSurface::GetYAxis() {
	return position->GetYAxis();
}

bool
ConicalSurface::Load(STEPWrapper *sw, SCLP23(Application_instance) *sse) {
	step=sw;
	id = sse->STEPfile_id;

	if ( !ElementarySurface::Load(step,sse) ) {
		cout << CLASSNAME << ":Error loading base class ::Surface." << endl;
		return false;
	}

	// need to do this for local attributes to makes sure we have
	// the actual entity and not a complex/supertype parent
	sse = step->getEntity(sse,ENTITYNAME);

	radius = step->getRealAttribute(sse,"radius");
	semi_angle = step->getRealAttribute(sse,"semi_angle");

	return true;
}

void
ConicalSurface::Print(int level) {
	TAB(level); cout << CLASSNAME << ":" << name << "(";
	cout << "ID:" << STEPid() << ")" << endl;

	TAB(level+1); cout << "radius: " << radius << endl;
	TAB(level+1); cout << "semi_angle: " << semi_angle << endl;

	ElementarySurface::Print(level+1);
}

STEPEntity *
ConicalSurface::Create(STEPWrapper *sw, SCLP23(Application_instance) *sse) {
	Factory::OBJECTS::iterator i;
	if ((i = Factory::FindObject(sse->STEPfile_id)) == Factory::objects.end()) {
		ConicalSurface *object = new ConicalSurface(sw,sse->STEPfile_id);

		Factory::AddObject(object);

		if (!object->Load(sw, sse)) {
			cerr << CLASSNAME << ":Error loading class in ::Create() method." << endl;
			delete object;
			return NULL;
		}
		return static_cast<STEPEntity *>(object);
	} else {
		return (*i).second;
	}
}
