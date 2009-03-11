#ifndef  SDAICLASSES_H
#define  SDAICLASSES_H
// This file was generated by fedex_plus.  You probably don't want to edit
// it since your modifications will be lost if fedex_plus is used to
// regenerate it.
/* $Id$  */ 
#include <schema.h>

// Schema:  SdaiEXAMPLE_SCHEMA
extern Schema *  s_example_schema;

// Types:
class SdaiColor_vars;
extern EnumTypeDescriptor 	*example_schemat_color;
typedef SCLP23(String) 	SdaiLabel;
extern TypeDescriptor 	*example_schemat_label;
typedef SCLP23(Real) 	SdaiPoint;
extern TypeDescriptor 	*example_schemat_point;
typedef SCLP23(Real) 	SdaiLength_measure;
extern TypeDescriptor 	*example_schemat_length_measure;

// Entities:
class SdaiPoly_line;
typedef SdaiPoly_line *  	SdaiPoly_lineH;
typedef SdaiPoly_line *  	SdaiPoly_line_ptr;
typedef SdaiPoly_line_ptr	SdaiPoly_line_var;
#define SdaiPoly_line__set 	SCLP23(DAObject__set)
#define SdaiPoly_line__set_var 	SCLP23(DAObject__set_var)
extern EntityDescriptor 	*example_schemae_poly_line;

class SdaiShape;
typedef SdaiShape *  	SdaiShapeH;
typedef SdaiShape *  	SdaiShape_ptr;
typedef SdaiShape_ptr	SdaiShape_var;
#define SdaiShape__set 	SCLP23(DAObject__set)
#define SdaiShape__set_var 	SCLP23(DAObject__set_var)
extern EntityDescriptor 	*example_schemae_shape;

class SdaiRectangle;
typedef SdaiRectangle *  	SdaiRectangleH;
typedef SdaiRectangle *  	SdaiRectangle_ptr;
typedef SdaiRectangle_ptr	SdaiRectangle_var;
#define SdaiRectangle__set 	SCLP23(DAObject__set)
#define SdaiRectangle__set_var 	SCLP23(DAObject__set_var)
extern EntityDescriptor 	*example_schemae_rectangle;

class SdaiSquare;
typedef SdaiSquare *  	SdaiSquareH;
typedef SdaiSquare *  	SdaiSquare_ptr;
typedef SdaiSquare_ptr	SdaiSquare_var;
#define SdaiSquare__set 	SCLP23(DAObject__set)
#define SdaiSquare__set_var 	SCLP23(DAObject__set_var)
extern EntityDescriptor 	*example_schemae_square;

class SdaiTriangle;
typedef SdaiTriangle *  	SdaiTriangleH;
typedef SdaiTriangle *  	SdaiTriangle_ptr;
typedef SdaiTriangle_ptr	SdaiTriangle_var;
#define SdaiTriangle__set 	SCLP23(DAObject__set)
#define SdaiTriangle__set_var 	SCLP23(DAObject__set_var)
extern EntityDescriptor 	*example_schemae_triangle;

class SdaiCircle;
typedef SdaiCircle *  	SdaiCircleH;
typedef SdaiCircle *  	SdaiCircle_ptr;
typedef SdaiCircle_ptr	SdaiCircle_var;
#define SdaiCircle__set 	SCLP23(DAObject__set)
#define SdaiCircle__set_var 	SCLP23(DAObject__set_var)
extern EntityDescriptor 	*example_schemae_circle;

class SdaiLine;
typedef SdaiLine *  	SdaiLineH;
typedef SdaiLine *  	SdaiLine_ptr;
typedef SdaiLine_ptr	SdaiLine_var;
#define SdaiLine__set 	SCLP23(DAObject__set)
#define SdaiLine__set_var 	SCLP23(DAObject__set_var)
extern EntityDescriptor 	*example_schemae_line;

class SdaiCartesian_point;
typedef SdaiCartesian_point *  	SdaiCartesian_pointH;
typedef SdaiCartesian_point *  	SdaiCartesian_point_ptr;
typedef SdaiCartesian_point_ptr	SdaiCartesian_point_var;
#define SdaiCartesian_point__set 	SCLP23(DAObject__set)
#define SdaiCartesian_point__set_var 	SCLP23(DAObject__set_var)
extern EntityDescriptor 	*example_schemae_cartesian_point;

#endif
