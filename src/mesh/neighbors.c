/*------------ -------------- -------- --- ----- ---   --       -            -
 *  wasora's mesh-related neighbor routines
 *
 *  Copyright (C) 2014--2016 jeremy theler
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

#include <string.h>
#include <gsl/gsl_math.h>

int mesh_count_common_nodes(element_t *e1, element_t *e2, int *nodes) {
  
  int i, j;
  int k = 0;
  
  
  for (i = 0; i < e1->type->nodes; i++) {
    for (j = 0; j < e2->type->nodes; j++) {
      if (e1->node[i] == e2->node[j]) {
        if (nodes != NULL) {
          nodes[k] = e1->node[i]->id;
        }
        k++;
      }
    }
  }
  
  return k;
  
}

element_t *mesh_find_element_volumetric_neighbor(element_t *element) {
  int j;
  element_list_item_t *associated_element;
  
  for (j = 0; j < element->type->nodes; j++) {
    LL_FOREACH(element->node[j]->associated_elements, associated_element) {
      if (element->type->dim == (associated_element->element->type->dim-1)) {  // los vecinos volumetricos
        if (mesh_count_common_nodes(element, associated_element->element, NULL) == element->type->nodes) {
          return associated_element->element;
        }
      }
    }
  }
  
  return NULL;
}

element_t *mesh_find_node_neighbor_of_dim(node_t *node, int dim) {
  int j;
  element_list_item_t *associated_element;
  
  LL_FOREACH(node->associated_elements, associated_element) {
    if (dim == associated_element->element->type->dim) {
      for (j = 0; j < associated_element->element->type->nodes; j++) {
        if (node->id == associated_element->element->node[j]->id) {
          return associated_element->element;
        }
      }
    }
  }
  
  return NULL;
}


int mesh_find_neighbors(mesh_t *mesh) {
  
  int i, j, l, m, n;
  int flag;
  int nodes[32]; // habra algun elemento que tenga mas de 32 nodos?
  element_list_item_t *associated_element;

  for (i = 0; i < mesh->n_cells; i++) {
  
    n = 0;
    mesh->cell[i].ifaces = calloc(mesh->cell[i].element->type->faces, sizeof(int *));
    
    for (j = 0; j < mesh->cell[i].element->type->nodes; j++) {
      LL_FOREACH(mesh->cell[i].element->node[j]->associated_elements, associated_element) {

        memset(nodes, 0, sizeof(nodes));
        l = associated_element->element->id;
        if (mesh_count_common_nodes(mesh->cell[i].element, &mesh->element[l-1], nodes) == mesh->cell[i].element->type->nodes_per_face) {

          flag = 1;
          for (m = 0; m < n; m++) {
            if (mesh->cell[i].ineighbor[m] == l) {
              flag = 0;
            }
          }
          
          if (flag) {
            if (n == mesh->cell[i].element->type->faces) {
              wasora_push_error_message("mesh inconsistency, element %d has at least one more neighbor than faces (%d)", mesh->cell[i].element->id, n);
              return WASORA_RUNTIME_ERROR;
            }

            mesh->cell[i].ifaces[n] = calloc(mesh->cell[i].element->type->nodes_per_face, sizeof(int));
            memcpy(mesh->cell[i].ifaces[n], nodes, sizeof(int)*mesh->cell[i].element->type->nodes_per_face);
            mesh->cell[i].ineighbor[n] = l;
          
            n++;
          }
        }
      }
    }
    
    if ((mesh->cell[i].n_neighbors = n) != mesh->cell[i].element->type->faces) {
      wasora_push_error_message("mesh inconsistency, element %d has less neighbors (%d) than faces (%d)", mesh->cell[i].element->id, n, mesh->cell[i].element->type->faces);
      return WASORA_RUNTIME_ERROR;
    }

  }

    
  return WASORA_RUNTIME_OK;
  
}

int mesh_fill_neighbors(mesh_t *mesh) {
  int i, j, k;
  double a[3], b[3], xi[3];
  double module;

  for (i = 0; i < mesh->n_cells; i++) {

    // holder para vecinos
    mesh->cell[i].neighbor = calloc(mesh->cell[i].n_neighbors, sizeof(struct neighbor_t));
    
    for (j = 0; j < mesh->cell[i].n_neighbors; j++) {
      
      // apuntador al elemento a partir del numero
      mesh->cell[i].neighbor[j].element = &mesh->element[mesh->cell[i].ineighbor[j]-1];
      
      // apuntador a la celda
      mesh->cell[i].neighbor[j].cell = mesh->cell[i].neighbor[j].element->cell;
      
      // calculamos las coordenadas de los nodos que definen la cara y del centro de la cara
      mesh->cell[i].neighbor[j].face_coord = calloc(mesh->cell[i].element->type->nodes_per_face, sizeof(double *));
      
      mesh->cell[i].neighbor[j].x_ij[0] = mesh->cell[i].neighbor[j].x_ij[1] = mesh->cell[i].neighbor[j].x_ij[2] = 0;
      for (k = 0; k < mesh->cell[i].element->type->nodes_per_face; k++) {
        mesh->cell[i].neighbor[j].face_coord[k] = mesh->node[mesh->cell[i].ifaces[j][k]-1].x;

        mesh->cell[i].neighbor[j].x_ij[0] += mesh->cell[i].neighbor[j].face_coord[k][0];
        mesh->cell[i].neighbor[j].x_ij[1] += mesh->cell[i].neighbor[j].face_coord[k][1];
        mesh->cell[i].neighbor[j].x_ij[2] += mesh->cell[i].neighbor[j].face_coord[k][2];        
      }
      mesh->cell[i].neighbor[j].x_ij[0] /= (double)mesh->cell[i].element->type->nodes_per_face;
      mesh->cell[i].neighbor[j].x_ij[1] /= (double)mesh->cell[i].element->type->nodes_per_face;
      mesh->cell[i].neighbor[j].x_ij[2] /= (double)mesh->cell[i].element->type->nodes_per_face;
      
      switch (mesh->bulk_dimensions) {
        case 1:
          if (mesh->cell[i].neighbor[j].face_coord[0][0] > mesh->cell[i].x[0]) {
            mesh->cell[i].neighbor[j].n_ij[0] = 1;
          } else {
            mesh->cell[i].neighbor[j].n_ij[0] = -1;
          }
          
          // area de la cara = en 1D es 1
          mesh->cell[i].neighbor[j].S_ij = 1;
          
        break;
        case 2:
          // OJO! esto funciona solo en el plano x-y
          // esta es la longitud de la cara
          module = mesh_subtract_module(mesh->cell[i].neighbor[j].face_coord[1], mesh->cell[i].neighbor[j].face_coord[0]);
        
          // proponemos uno de los dos vectores normales
          mesh->cell[i].neighbor[j].n_ij[0] = -(mesh->cell[i].neighbor[j].face_coord[1][1]-mesh->cell[i].neighbor[j].face_coord[0][1])/module;
          mesh->cell[i].neighbor[j].n_ij[1] =  (mesh->cell[i].neighbor[j].face_coord[1][0]-mesh->cell[i].neighbor[j].face_coord[0][0])/module;
          mesh->cell[i].neighbor[j].n_ij[2] = 0;
          
          // y vemos si el producto interno con el centro de la celda es positivo
          // si lo es, elegimos mal el vector normal y lo damos vuelta
          if (mesh_subtract_dot(mesh->cell[i].x, mesh->cell[i].neighbor[j].x_ij, mesh->cell[i].neighbor[j].n_ij) > 0) {
            mesh->cell[i].neighbor[j].n_ij[0] = -mesh->cell[i].neighbor[j].n_ij[0];
            mesh->cell[i].neighbor[j].n_ij[1] = -mesh->cell[i].neighbor[j].n_ij[1];
          }

          // area de la cara = longitud del segmento definido por dos puntos
          mesh->cell[i].neighbor[j].S_ij = gsl_hypot3(mesh->cell[i].neighbor[j].face_coord[0][0]-mesh->cell[i].neighbor[j].face_coord[1][0],
                                                      mesh->cell[i].neighbor[j].face_coord[0][1]-mesh->cell[i].neighbor[j].face_coord[1][1],
                                                      mesh->cell[i].neighbor[j].face_coord[0][2]-mesh->cell[i].neighbor[j].face_coord[1][2]);
          
        break;
        case 3:
          a[0] = mesh->cell[i].neighbor[j].face_coord[1][0] - mesh->cell[i].neighbor[j].face_coord[0][0];
          a[1] = mesh->cell[i].neighbor[j].face_coord[1][1] - mesh->cell[i].neighbor[j].face_coord[0][1];
          a[2] = mesh->cell[i].neighbor[j].face_coord[1][2] - mesh->cell[i].neighbor[j].face_coord[0][2];
          b[0] = mesh->cell[i].neighbor[j].face_coord[2][0] - mesh->cell[i].neighbor[j].face_coord[0][0];
          b[1] = mesh->cell[i].neighbor[j].face_coord[2][1] - mesh->cell[i].neighbor[j].face_coord[0][1];
          b[2] = mesh->cell[i].neighbor[j].face_coord[2][2] - mesh->cell[i].neighbor[j].face_coord[0][2];
          mesh_normalized_cross(a, b, mesh->cell[i].neighbor[j].n_ij);
          
          // y vemos si el producto interno con el centro de la celda es positivo
          // si lo es, elegimos mal el vector normal y lo damos vuelta
          if (mesh_subtract_dot(mesh->cell[i].x, mesh->cell[i].neighbor[j].x_ij, mesh->cell[i].neighbor[j].n_ij) > 0) {
            mesh->cell[i].neighbor[j].n_ij[0] = -mesh->cell[i].neighbor[j].n_ij[0];
            mesh->cell[i].neighbor[j].n_ij[1] = -mesh->cell[i].neighbor[j].n_ij[1];
            mesh->cell[i].neighbor[j].n_ij[2] = -mesh->cell[i].neighbor[j].n_ij[2];
          }
          
          // el area de la cara
          // los vectores a y b ya los tenemos
          mesh_cross(a, b, xi);
          mesh->cell[i].neighbor[j].S_ij = 0.5 * gsl_hypot3(xi[0], xi[1], xi[2]);
          
          if (mesh->cell[i].element->type->nodes_per_face == 4) {
            // si la cara es un cuadrangulo, entonces sumamos el otro triangulito
            a[0] = mesh->cell[i].neighbor[j].face_coord[3][0] - mesh->cell[i].neighbor[j].face_coord[0][0];
            a[1] = mesh->cell[i].neighbor[j].face_coord[3][1] - mesh->cell[i].neighbor[j].face_coord[0][1];
            a[2] = mesh->cell[i].neighbor[j].face_coord[3][2] - mesh->cell[i].neighbor[j].face_coord[0][2];
            mesh_cross(a, b, xi);
            mesh->cell[i].neighbor[j].S_ij += 0.5 * gsl_hypot3(xi[0], xi[1], xi[2]);
          }
        break;
      }
    }
  }
  
  return WASORA_RUNTIME_OK;
  
  
}
