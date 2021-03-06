/*------------ -------------- -------- --- ----- ---   --       -            -
 *  wasora's mesh-related second-order quadrangle element routines
 *
 *  Copyright (C) 2017 C.P.Camusso.
 *  Copyright (C) 2018 jeremy theler
 *
 *  This file is part of wasora.
 *
 *  wasora is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  wasora is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with wasora.  If not, see <http://www.gnu.org/licenses/>.
 *------------------- ------------  ----    --------  --     -       -         -
 */
#include <wasora.h>

#include <math.h>

// --------------------------------------------------------------
// cuadrangulo de cuatro nodos
// --------------------------------------------------------------
int mesh_quad8_init(void) {
  
  element_type_t *element_type;
  int j;
  
  element_type = &wasora_mesh.element_type[ELEMENT_TYPE_QUADRANGLE8];
  element_type->name = strdup("quad8");
  element_type->id = ELEMENT_TYPE_QUADRANGLE8;
  element_type->dim = 2;
  element_type->order = 2;
  element_type->nodes = 8;
  element_type->faces = 4;
  element_type->nodes_per_face = 3;
  element_type->h = mesh_quad8_h;
  element_type->dhdr = mesh_quad8_dhdr;
  element_type->point_in_element = mesh_point_in_quadrangle;
  element_type->element_volume = mesh_quad_vol;

  // coordenadas de los nodos
/*
 Quadrangle8:   

 3-----6-----2  
 |           |  
 |           |  
 7           5  
 |           |  
 |           |  
 0-----4-----1      
*/     
  element_type->node_coords = calloc(element_type->nodes, sizeof(double *));
  element_type->node_parents = calloc(element_type->nodes, sizeof(node_relative_t *));  
  for (j = 0; j < element_type->nodes; j++) {
    element_type->node_coords[j] = calloc(element_type->dim, sizeof(double));  
  }
  
  element_type->first_order_nodes++;
  element_type->node_coords[0][0] = -1;
  element_type->node_coords[0][1] = -1;
  
  element_type->first_order_nodes++;
  element_type->node_coords[1][0] = +1;  
  element_type->node_coords[1][1] = -1;
  
  element_type->first_order_nodes++;
  element_type->node_coords[2][0] = +1;  
  element_type->node_coords[2][1] = +1;

  element_type->first_order_nodes++;
  element_type->node_coords[3][0] = -1;
  element_type->node_coords[3][1] = +1;

 
  wasora_mesh_add_node_parent(&element_type->node_parents[4], 0);
  wasora_mesh_add_node_parent(&element_type->node_parents[4], 1);
  wasora_mesh_compute_coords_from_parent(element_type, 4);
  
  wasora_mesh_add_node_parent(&element_type->node_parents[5], 1);
  wasora_mesh_add_node_parent(&element_type->node_parents[5], 2);
  wasora_mesh_compute_coords_from_parent(element_type, 5); 
 
  wasora_mesh_add_node_parent(&element_type->node_parents[6], 2);
  wasora_mesh_add_node_parent(&element_type->node_parents[6], 3);
  wasora_mesh_compute_coords_from_parent(element_type, 6);
 
  wasora_mesh_add_node_parent(&element_type->node_parents[7], 3);
  wasora_mesh_add_node_parent(&element_type->node_parents[7], 0);
  wasora_mesh_compute_coords_from_parent(element_type, 7);

  mesh_quad_gauss4_init(element_type);
  
  return WASORA_RUNTIME_OK;    
}

//Taken from https://www.code-aster.org/V2/doc/default/fr/man_r/r3/r3.01.01.pdf
//The aster node ordering of aster and gmsh are equal.
double mesh_quad8_h(int j, double *vec_r) {
  double r = vec_r[0];
  double s = vec_r[1];

  switch (j) {
    case 0:
      return (1-r)*(1-s)*(-1-r-s)/4;
      break;
    case 1:
      return (1+r)*(1-s)*(-1+r-s)/4;
      break;
    case 2:
      return (1+r)*(1+s)*(-1+r+s)/4;
      break;
    case 3:
      return (1-r)*(1+s)*(-1-r+s)/4;
      break;
    case 4:
      return (1-r*r)*(1-s)/2;
      break;
    case 5:
      return (r+1)*(1-s*s)/2;
      break;
    case 6:
      return (1-r*r)*(s+1)/2;
      break;
    case 7:
      return (1-r)*(1-s*s)/2;
      break;
  }

  return 0;

}

double mesh_quad8_dhdr(int j, int m, double *vec_r) {
  double r = vec_r[0];
  double s = vec_r[1];

  switch(j) {
    case 0:
      if (m == 0) {
        return (1-s)*(2*r+s)/4;
      } else {
        return (1-r)*(r+2*s)/4;
      }
      break;
    case 1:
      if (m == 0) {
        return (1-s)*(2*r-s)/4;
      } else {
        return -(1+r)*(r-2*s)/4;
      }
      break;
    case 2:
      if (m == 0) {
        return (1+s)*(2*r+s)/4;
      } else {
        return (1+r)*(r+2*s)/4;
      }
      break;
    case 3:
      if (m == 0) {
        return -(1+s)*(-2*r+s)/4;
      } else {
        return (1-r)*(-r+2*s)/4;
      }
      break;
    case 4:
      if (m == 0) {
        return -r*(1-s);
      } else {
        return -(1-r*r)/2;
      }
      break;
    case 5:
      if (m == 0) {
        return (1-s*s)/2;
      } else {
        return -s*(1+r);
      }
      break;
    case 6:
      if (m == 0) {
        return -r*(1+s);
      } else {
        return (1-r*r)/2;
      }
      break;
    case 7:
      if (m == 0) {
        return -(1-s*s)/2;
      } else {
        return -s*(1-r);;
      }
      break;

  }

  return 0;

}
