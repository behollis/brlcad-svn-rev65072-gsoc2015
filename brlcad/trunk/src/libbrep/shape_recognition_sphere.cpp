#include "common.h"

#include <set>
#include <map>

#include "bu/log.h"
#include "bu/str.h"
#include "bu/malloc.h"
#include "shape_recognition.h"

int
subbrep_is_sphere(struct subbrep_object_data *data, fastf_t cyl_tol)
{
    std::set<int>::iterator f_it;
    std::set<int> spherical_surfaces;
    // First, check surfaces.  If a surface is anything other than a sphere,
    // the verdict is no.
    for (int i = 0; i < data->faces_cnt; i++) {
	int f_ind = data->faces[i];
        int surface_type = (int)GetSurfaceType(data->brep->m_F[f_ind].SurfaceOf(), NULL);
        switch (surface_type) {
            case SURFACE_SPHERE:
            case SURFACE_SPHERICAL_SECTION:
                spherical_surfaces.insert(f_ind);
                break;
            default:
                return 0;
                break;
        }
    }

    // Second, check if all spherical surfaces share the same center and radius.
    ON_Sphere sph;
    ON_Surface *cs = data->brep->m_F[*spherical_surfaces.begin()].SurfaceOf()->Duplicate();
    cs->IsSphere(&sph, BREP_SPHERICAL_TOL);
    delete cs;
    for (f_it = spherical_surfaces.begin(); f_it != spherical_surfaces.end(); f_it++) {
	ON_Sphere f_sph;
	ON_Surface *fcs = data->brep->m_F[(*f_it)].SurfaceOf()->Duplicate();
	fcs->IsSphere(&sph, BREP_SPHERICAL_TOL);
	delete fcs;
	if (f_sph.Center().DistanceTo(sph.Center()) > BREP_SPHERICAL_TOL) return 0;
	if (!NEAR_ZERO(f_sph.Radius() - sph.Radius(), BREP_SPHERICAL_TOL)) return 0;
    }

    // TODO - devise other tests necessary to make sure we have a closed sphere,
    // if any are needed.  Maybe rule out anything with linear edges?


    data->type = SPHERE;
    struct csg_object_params * obj;
    BU_GET(obj, struct csg_object_params);

    data->params->bool_op= 'u';  // TODO - not always union
    data->params->origin[0] = sph.Center().x;
    data->params->origin[1] = sph.Center().y;
    data->params->origin[2] = sph.Center().z;
    data->params->radius = sph.Radius();

    return 1;
}

/* Return -1 if the sphere face is pointing in toward the center,
 * 1 if it is pointing out, and 0 if there is some other problem */
int
negative_sphere(struct subbrep_object_data *data, int face_index, double sph_tol) {
    const ON_Surface *surf = data->brep->m_F[face_index].SurfaceOf();
    ON_Sphere sph;
    ON_Surface *cs = surf->Duplicate();
    cs->IsSphere(&sph, sph_tol);
    delete cs;

    ON_3dPoint pnt;
    ON_3dVector normal;
    double u = surf->Domain(0).Mid();
    double v = surf->Domain(1).Mid();
    if (!surf->EvNormal(u, v, pnt, normal)) return 0;

    ON_3dVector vect = pnt - sph.Center();
    double dotp = ON_DotProduct(vect, normal);

    if (NEAR_ZERO(dotp, 0.000001)) return 0;
    if (dotp < 0) return 1;
    return -1;
}


int
sphere_csg(struct subbrep_object_data *data, fastf_t sph_tol)
{
    std::set<int> planar_surfaces;
    std::set<int> spherical_surfaces;
    std::set<int>::iterator f_it;
    std::cout << "processing spherical surface: ";
    for (int i = 0; i < data->faces_cnt; i++) {
	int f_ind = data->faces[i];
        int surface_type = (int)GetSurfaceType(data->brep->m_F[f_ind].SurfaceOf(), NULL);
        switch (surface_type) {
            case SURFACE_PLANE:
                planar_surfaces.insert(f_ind);
                break;
            case SURFACE_SPHERE:
            case SURFACE_SPHERICAL_SECTION:
                spherical_surfaces.insert(f_ind);
		break;
            default:
		std::cout << "what???\n";
                return 0;
                break;
        }
    }
    data->params->bool_op = 'u'; // Initialize to union

    // Check for multiple spheres.
    ON_Sphere sph;
    ON_Surface *cs = data->brep->m_F[*spherical_surfaces.begin()].SurfaceOf()->Duplicate();
    cs->IsSphere(&sph, BREP_SPHERICAL_TOL);
    delete cs;
    for (f_it = spherical_surfaces.begin(); f_it != spherical_surfaces.end(); f_it++) {
	ON_Sphere f_sph;
	ON_Surface *fcs = data->brep->m_F[(*f_it)].SurfaceOf()->Duplicate();
	fcs->IsSphere(&f_sph, BREP_SPHERICAL_TOL);
	delete fcs;
	if (f_sph.Center().DistanceTo(sph.Center()) > BREP_SPHERICAL_TOL) return 0;
	if (!NEAR_ZERO(f_sph.Radius() - sph.Radius(), BREP_SPHERICAL_TOL)) return 0;
    }


    // Count the number of vertices associated with this subbrep.
    std::set<int> verts;
    std::set<int>::iterator v_it;
    for (int i = 0; i < data->edges_cnt; i++) {
	const ON_BrepEdge *edge = &(data->brep->m_E[data->edges[i]]);
	verts.insert(edge->Vertex(0)->m_vertex_index);
	verts.insert(edge->Vertex(1)->m_vertex_index);
    }

    if (verts.size() == 1) {
	std::cout << "Only one vertex - probably a circular trim defining a planar face?\n";
	return 0;
    }

    if (verts.size() == 2 && data->edges_cnt == 1) {
	std::cout << "Two vertices, one edge - probably whole sphere\n";
	return 0;
    }

    if (verts.size() == 2 && data->edges_cnt >= 2) {
	std::cout << "Two vertices, more than one edge.  Either a two plane situation or two arcs forming a one plane situation\n";
	return 0;
    }

    if (verts.size() == 3) {
	std::cout << "Three vertices\n";
	// Need the planes of any non-linear edges and the plane of the three verts.
	ON_SimpleArray<const ON_BrepVertex *> sph_verts(3);
	std::set<int> trims;
	for (v_it = verts.begin(); v_it != verts.end(); v_it++) {
	    sph_verts.Append(&(data->brep->m_V[*v_it]));
	}
	ON_3dPoint p1 = sph_verts[0]->Point();
	ON_3dPoint p2 = sph_verts[1]->Point();
	ON_3dPoint p3 = sph_verts[2]->Point();
	ON_Plane back_plane(p1, p2, p3);
	ON_3dPoint bpc = (p1 + p2 + p3)/3;

	// In order to determine which part of the sphere is the "positive"
	// part, assemble a second plane from the mid points of the edges.
	// If any of these midpoints are on the plane of the vertices this
	// degenerates into a one or two plane case, but if all three are
	// off of the back plane the vector between them will give us the
	// orientation we need.
	ON_SimpleArray<ON_3dPoint> mid_pnts(3);
	for (int i = 0; i < data->edges_cnt; i++) {
	    const ON_BrepEdge *edge = &(data->brep->m_E[data->edges[i]]);
	    const ON_Curve *curve = edge->EdgeCurveOf();
	    ON_3dPoint mid_pnt = curve->PointAt(curve->Domain().Mid());
	    mid_pnts.Append(mid_pnt);
	}
	ON_Plane front_plane(mid_pnts[0], mid_pnts[1], mid_pnts[2]);
	ON_3dPoint fpc = (mid_pnts[0] + mid_pnts[1] + mid_pnts[2])/3;

	// Using the above info, set the back_plane normal to the correct
	// direction needed for defining a face in a B-Rep bounding the
	// spherical sub-volume.
	ON_3dVector pv = fpc - bpc;
	if (ON_DotProduct(pv, back_plane.Normal()) > 0) back_plane.Flip();

	// Then, construct planes for each arc from the three points.  Use
	// the vectors between the center point of the back plane
	// and the midpoints of each edge to determine if the plane normal
	// should be flipped - the plane normals of the edge planes shouldn't
	// be pointing back towards the back face center point.
	ON_SimpleArray<ON_Plane> edge_planes(3);
	for (int i = 0; i < data->edges_cnt; i++) {
	    const ON_BrepEdge *edge = &(data->brep->m_E[data->edges[i]]);
	    const ON_Curve *curve = edge->EdgeCurveOf();
	    ON_3dPoint start_pnt = curve->PointAtStart();
	    ON_3dPoint mid_pnt = curve->PointAt(curve->Domain().Mid());
	    ON_3dPoint end_pnt = curve->PointAtEnd();
	    ON_Plane new_plane(start_pnt, mid_pnt, end_pnt);
	    ON_3dVector guide = mid_pnt - bpc;
	    if (ON_DotProduct(guide, new_plane.Normal()) < 0) back_plane.Flip();
	    edge_planes.Append(new_plane);
	}

	// A planar face parallel to the back plane must also be added to
	// the parent planer brep, if there is one.
	if (!data->is_island && data->parent) {
	    if (!data->parent->planar_obj) {
		subbrep_planar_init(data);
	    }
	    std::cout << "add sph plane\n";
	    subbrep_add_planar_face(data->parent, &back_plane, &sph_verts);
	}

	// The planes each define an arb8 (4 all together) that carve the
	// sub-sphere shape out of the parent sphere with subtractions.
	// Using the normals, center points, edge and sphere information,
	// construct the 4 arbs.


	return 0;
    }

    if (verts.size() >= 3) {
	std::cout << "Complex situation.\n";
	return 0;
    }

}

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
