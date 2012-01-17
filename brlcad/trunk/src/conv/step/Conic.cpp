/*                 Conic.cpp
 * BRL-CAD
 *
 * Copyright (c) 1994-2012 United States Government as represented by
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
/** @file step/Conic.cpp
 *
 * Routines to convert STEP "Conic" to BRL-CAD BREP
 * structures.
 *
 */

#include "STEPWrapper.h"
#include "Factory.h"
#include "Axis2Placement.h"

#include "Conic.h"

#define CLASSNAME "Conic"
#define ENTITYNAME "Conic"
string Conic::entityname = Factory::RegisterClass(ENTITYNAME,(FactoryMethod)Conic::Create);

Conic::Conic() {
    step = NULL;
    id = 0;
    position = NULL;
}

Conic::Conic(STEPWrapper *sw,int step_id) {
    step = sw;
    id = step_id;
    position = NULL;
}

Conic::~Conic() {
    delete position;
}

const double *
Conic::GetOrigin() {
    return position->GetOrigin();
}

const double *
Conic::GetNormal() {
    return position->GetNormal();
}

const double *
Conic::GetXAxis() {
    return position->GetXAxis();
}

const double *
Conic::GetYAxis() {
    return position->GetYAxis();
}

bool
Conic::Load(STEPWrapper *sw,SCLP23(Application_instance) *sse) {
    step=sw;
    id = sse->STEPfile_id;

    if ( !GeometricRepresentationItem::Load(step,sse) ) {
	std::cout << CLASSNAME << ":Error loading base class ::GeometricRepresentationItem." << std::endl;
	return false;
    }

    // need to do this for local attributes to makes sure we have
    // the actual entity and not a complex/supertype parent
    sse = step->getEntity(sse,ENTITYNAME);

    if (position == NULL) {
	SCLP23(Select) *select = step->getSelectAttribute(sse,"position");
	if (select) {
	    //if (select->CurrentUnderlyingType() == config_control_designt_axis2_placement) {
	    Axis2Placement *aA2P = new Axis2Placement();

	    position = aA2P;
	    if (!aA2P->Load(step,select)) {
		std::cout << CLASSNAME << ":Error loading select Axis2Placement from Conic." << std::endl;
		return false;
	    }

	    //} else {
	    //	std::cout << CLASSNAME << ":Unexpected select type for 'position' : " << select->UnderlyingTypeName() << std::endl;
	    //	return false;
	    //}
	}
    }

    return true;
}

void
Conic::Print(int level) {
    TAB(level); std::cout << CLASSNAME << ":" << name << "(";
    std::cout << "ID:" << STEPid() << ")" << std::endl;

    TAB(level); std::cout << "Attributes:" << std::endl;
    TAB(level+1); std::cout << "position:" << std::endl;
    position->Print(level+1);

    TAB(level); std::cout << "Inherited Attributes:" << std::endl;
    GeometricRepresentationItem::Print(level+1);

}

STEPEntity *
Conic::Create(STEPWrapper *sw, SCLP23(Application_instance) *sse) {
    Factory::OBJECTS::iterator i;
    if ((i = Factory::FindObject(sse->STEPfile_id)) == Factory::objects.end()) {
	Conic *object = new Conic(sw,sse->STEPfile_id);

	Factory::AddObject(object);

	if (!object->Load(sw, sse)) {
	    std::cerr << CLASSNAME << ":Error loading class in ::Create() method." << std::endl;
	    delete object;
	    return NULL;
	}
	return static_cast<STEPEntity *>(object);
    } else {
	return (*i).second;
    }
}

bool
Conic::LoadONBrep(ON_Brep *brep)
{
    std::cerr << "Error: ::LoadONBrep(ON_Brep *brep<" << std::hex << brep << std::dec << ">) not implemented for " << entityname << std::endl;
    return false;
}

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
