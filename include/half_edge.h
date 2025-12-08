#ifndef HALF_EDGE_H
#define HALF_EDGE_H

#include <stdlib.h>
#include "obj_loader.h"
#include "matrix_math.h"

struct HE_Face;
struct HE_HalfEdge;
struct HE_Vertex;

typedef struct HE_Vertex{
        float x;
        float y;
        float z;
        unsigned int inc_edge_ID;
}HE_Vertex;

typedef struct HE_Face{
        unsigned int edge_ID;
}HE_Face;

typedef struct HE_HalfEdge{
        unsigned int origin_vertex_ID;
        unsigned int twin_edge_ID;
        unsigned int inc_face_ID;
        unsigned int prvsEdge_ID;
        unsigned int nextEdge_ID;
}HE_HalfEdge;

typedef struct HE_Vertex_Array{
        HE_Vertex* array;
        unsigned int size;
}HE_Vertex_Array;

typedef struct HE_Face_Array{
        HE_Face* array;
        unsigned int size;
}HE_Face_Array;

typedef struct HE_Edge_Array{
        HE_HalfEdge* array;
        unsigned int size;
}HE_Edge_Array;

typedef struct{
        HE_Vertex_Array vertex_array;
        HE_Face_Array face_array;
        HE_Edge_Array edge_array;
}HE_Object;

void HE_applyTransform(mat4 m, HE_Object o){
        unsigned int size = o.vertex_array.size;
        for(int i = 0; i < size; i++){
                float x = o.vertex_array.array[i].x;
                float y = o.vertex_array.array[i].y;
                float z = o.vertex_array.array[i].z;
                vec3 vertex = {x, y, z};
                mm_mat4_mulv3(m, vertex, vertex);
                o.vertex_array.array[i].x = vertex[0];
                o.vertex_array.array[i].y = vertex[1];
                o.vertex_array.array[i].z = vertex[2];
        }
}

void HE_vertexArray_Push(HE_Vertex_Array* verArray, float x, float y, float z, unsigned int inc_edge_ID){
        if(verArray->size == 0){
                verArray->array = (HE_Vertex*)malloc(sizeof(HE_Vertex));
        }else{
                verArray->array = (HE_Vertex*)realloc(verArray->array, sizeof(HE_Vertex)*(verArray->size+1));
        }
        verArray->array[verArray->size].x = x;
        verArray->array[verArray->size].y = y;
        verArray->array[verArray->size].z = z;
        verArray->array[verArray->size].inc_edge_ID = inc_edge_ID;
        verArray->size++;
}

void HE_edgeArray_Push(HE_Edge_Array* edgeArray, unsigned int origin_vertex_ID, unsigned int twin_ID, unsigned int inc_face_ID, unsigned int nextEdge_ID, unsigned int prvsEdge_ID){
        if(edgeArray->size == 0){
                edgeArray->array = (HE_HalfEdge*)malloc(sizeof(HE_HalfEdge));
        }else{
                edgeArray->array = (HE_HalfEdge*)realloc(edgeArray->array, sizeof(HE_HalfEdge)*(edgeArray->size+1));
        }
        edgeArray->array[edgeArray->size].origin_vertex_ID = origin_vertex_ID;
        edgeArray->array[edgeArray->size].twin_edge_ID = twin_ID;
        edgeArray->array[edgeArray->size].inc_face_ID = inc_face_ID;
        edgeArray->array[edgeArray->size].prvsEdge_ID = prvsEdge_ID;
        edgeArray->array[edgeArray->size].nextEdge_ID = nextEdge_ID;
        edgeArray->size++;
}

void HE_faceArray_Push(HE_Face_Array* faceArray, unsigned int edge_ID){
        if(faceArray->size == 0){
                faceArray->array = (HE_Face*)malloc(sizeof(HE_Face));
        }else{
                faceArray->array = (HE_Face*)realloc(faceArray->array, sizeof(HE_Face)*(faceArray->size+1));
        }
        faceArray->array[faceArray->size].edge_ID = edge_ID;
        faceArray->size++;
}

unsigned int* HE_get_object_single_face_as_array(HE_Object object, int* size, const unsigned int face){
        HE_Face_Array faces = object.face_array;
        HE_Edge_Array edges = object.edge_array;
        int counter = 0;

        int starting_edge = faces.array[face].edge_ID;
        int starting_vertex = edges.array[starting_edge].origin_vertex_ID;
        counter++;
        int next_edge = edges.array[starting_edge].nextEdge_ID;
        int next_vertex = edges.array[next_edge].origin_vertex_ID;
        
        while(starting_edge != next_edge){
                next_edge = edges.array[next_edge].nextEdge_ID;
                next_vertex = edges.array[next_edge].origin_vertex_ID;
                counter++;
        }


        unsigned int* output = (unsigned int*)malloc(sizeof(unsigned int)*counter);
        counter = 0;

        starting_edge = faces.array[face].edge_ID;
        starting_vertex = edges.array[starting_edge].origin_vertex_ID;
        output[counter] = starting_vertex;
        counter++;
        next_edge = edges.array[starting_edge].nextEdge_ID;
        next_vertex = edges.array[next_edge].origin_vertex_ID;
        
        while(starting_edge != next_edge){
                output[counter] = next_vertex;
                next_edge = edges.array[next_edge].nextEdge_ID;
                next_vertex = edges.array[next_edge].origin_vertex_ID;
                counter++;
        }
        *size = counter;
        return output;
}

unsigned int* HE_get_object_faces_as_array(HE_Object object, int* size){
        HE_Face_Array faces = object.face_array;
        HE_Edge_Array edges = object.edge_array;
        int counter = 0;
        for(int i = 0; i < faces.size; i++){
                int starting_edge = faces.array[i].edge_ID;
                int starting_vertex = edges.array[starting_edge].origin_vertex_ID;
                counter++;
                int next_edge = edges.array[starting_edge].nextEdge_ID;
                int next_vertex = edges.array[next_edge].origin_vertex_ID;
                
                while(starting_edge != next_edge){
                        next_edge = edges.array[next_edge].nextEdge_ID;
                        next_vertex = edges.array[next_edge].origin_vertex_ID;
                        counter++;
                }
        }
        unsigned int* output = (unsigned int*)malloc(sizeof(unsigned int)*counter);
        counter = 0;
        for(int i = 0; i < faces.size; i++){
                int starting_edge = faces.array[i].edge_ID;
                int starting_vertex = edges.array[starting_edge].origin_vertex_ID;
                output[counter] = starting_vertex;
                counter++;
                int next_edge = edges.array[starting_edge].nextEdge_ID;
                int next_vertex = edges.array[next_edge].origin_vertex_ID;
                
                while(starting_edge != next_edge){
                        output[counter] = next_vertex;
                        next_edge = edges.array[next_edge].nextEdge_ID;
                        next_vertex = edges.array[next_edge].origin_vertex_ID;
                        counter++;
                }
        }
        *size = counter;
        return output;
}

int HE_get_next_edge(HE_Edge_Array edge_array, int index){   // NEEDS REFACTORING
        int starting_edge = index;
        int next_edge_ID = edge_array.array[index].nextEdge_ID;
        if(next_edge_ID != -1)
                return next_edge_ID;

        int edge_twin = edge_array.array[starting_edge].twin_edge_ID;
        int edge_twin_prev = edge_array.array[edge_twin].prvsEdge_ID;
        int edge_twin_prev_twin = edge_array.array[edge_twin_prev].twin_edge_ID; // Poss Candidate

        int candidate_prev = edge_array.array[edge_twin_prev_twin].prvsEdge_ID;
        if(candidate_prev == -1){
                return edge_twin_prev_twin;
        }
        while(edge_twin_prev != starting_edge){
                edge_twin_prev = edge_array.array[edge_twin_prev_twin].prvsEdge_ID;
                edge_twin_prev_twin = edge_array.array[edge_twin_prev].twin_edge_ID; // Poss Candidate
                                                                                         //
                int candidate_prev = edge_array.array[edge_twin_prev_twin].prvsEdge_ID;
                if(candidate_prev == -1){
                        return edge_twin_prev_twin;
                }
        }
        return -1;
}

static void HE_load_vertices(const OBJ object, HE_Object* output){
        float x, y, z;
        HE_Vertex_Array verArray  = {NULL, 0};

        // Carrega os vértices
        for(int i = 0; i < object.vertex.size; i++){
                x = object.vertex.verts[i].x;
                y = object.vertex.verts[i].y;
                z = object.vertex.verts[i].z;
                HE_vertexArray_Push(&verArray, x, y, z, 0);
        }

        output->vertex_array = verArray;
        return;
}

static void HE_center_model(HE_Object* object){
        HE_Vertex_Array verArray = object->vertex_array;
        // Centering model in local space
        float sumX = 0;
        float sumY = 0;
        float sumZ = 0;
        for(int i = 0; i < verArray.size; i++){
                sumX += verArray.array[i].x; 
                sumY += verArray.array[i].y; 
                sumZ += verArray.array[i].z; 
        }
        for(int i = 0; i < verArray.size; i++){
                verArray.array[i].x -= (sumX/verArray.size);
                verArray.array[i].y -= (sumY/verArray.size);
                verArray.array[i].z -= (sumZ/verArray.size);
        }
        return;
}

static void HE_generate_edge_and_face_array(const OBJ object, HE_Object* ret){
        HE_Vertex_Array verArray = ret->vertex_array;
        // -----------
        int conMap [verArray.size][verArray.size];

        for(int i = 0; i < verArray.size; i++)
                for(int j = 0; j < verArray.size; j++)
                        conMap[i][j] = -1;


        HE_Face_Array   faceArray = {NULL, 0};
        HE_Edge_Array   edgeArray = {NULL, 0};
        int* verts;
        int edge_counter = 0;
        for(int i = 0; i < object.face.size; i++){
                // Getting verts from this face
                int face_size = object.face.array[i].size; // How many verts on this face
                verts = (int*) malloc(sizeof(int)*face_size);
                for(int j = 0; j < face_size; j++){
                        verts[j] = object.face.array[i].vertex_ID[j];
                }
                for(int j = 0; j < face_size; j++){
                        conMap[verts[j]-1][verts[(j+1)%face_size]-1] = edge_counter++;
                }

                // Making new face with verts
                HE_faceArray_Push(&faceArray, edgeArray.size);
                int index = edgeArray.size;
                for(int j = 0; j < face_size; j++){
                        verArray.array[verts[j]-1].inc_edge_ID = index+j;
                }

                for(int j = 0; j < face_size; j++){
                        HE_edgeArray_Push(&edgeArray, verts[j]-1, 0, faceArray.size-1, index+((j+1)%face_size), index+((j+2)%face_size));
                }
                free(verts);
        }
        // Determina os twins dos edges a partir do mapa de relações
        // Caso não tenha twin, cria um edge novo
        for(int index = 0; index < edgeArray.size; index++){
                HE_HalfEdge edgeData = edgeArray.array[index];
                for(int i = 0; i < verArray.size; i++){
                        for(int j = 0; j < verArray.size; j++){
                                if(conMap[i][j] != index)
                                        continue;
                                if(conMap[j][i] == -1){
                                        HE_edgeArray_Push(&edgeArray, j, index, -1, -1, -1);
                                        conMap[j][i] = edgeArray.size-1;
                                }
                                edgeArray.array[index].twin_edge_ID = conMap[j][i];
                        }
                }
        }

        // Percorre os edges por relações para encontrar o previous e o next
        for(int index = 0; index < edgeArray.size; index++){
                HE_HalfEdge edgeData = edgeArray.array[index];
                HE_HalfEdge* array = edgeArray.array;
                if(edgeData.nextEdge_ID == -1){
                        int nextEdge_ID = HE_get_next_edge(edgeArray, index);
                        edgeArray.array[index].nextEdge_ID = nextEdge_ID;
                        edgeArray.array[nextEdge_ID].prvsEdge_ID = index;
                }
        }
        ret->edge_array   = edgeArray;
        ret->face_array   = faceArray;
        return;
}

void HE_copy_object(const HE_Object source, HE_Object* dest){
        float x, y, z;
        int nextID;
        HE_Vertex_Array vertArray = {NULL, 0};
        HE_Edge_Array   edgeArray = {NULL, 0};
        HE_Face_Array   faceArray = {NULL, 0};

        for(int i = 0; i < source.vertex_array.size; i++){
                x = source.vertex_array.array[i].x;
                y = source.vertex_array.array[i].y;
                z = source.vertex_array.array[i].z;
                nextID = source.vertex_array.array[i].inc_edge_ID;
                HE_vertexArray_Push(&vertArray, x, y, z, nextID);
        }
        int next, prvs, twin, inc, origin;
        for(int i = 0; i < source.edge_array.size; i++){
                origin = source.edge_array.array[i].origin_vertex_ID;
                twin = source.edge_array.array[i].twin_edge_ID;
                inc  = source.edge_array.array[i].inc_face_ID;
                next = source.edge_array.array[i].nextEdge_ID;
                prvs = source.edge_array.array[i].prvsEdge_ID;
                HE_edgeArray_Push(&edgeArray, origin, twin, inc, next, prvs);
        }
        int edgeID;
        for(int i = 0; i < source.face_array.size; i++){
                edgeID = source.face_array.array[i].edge_ID;
                HE_faceArray_Push(&faceArray, edgeID);
        }
        dest->vertex_array = vertArray;
        dest->edge_array   = edgeArray;
        dest->face_array   = faceArray;

          // PRINTS
        if(1){
        printf("Output from function HE_copy_object\n");
        printf("Vertices\n");
        for(int i = 0; i < vertArray.size; i++){
                HE_Vertex vert = vertArray.array[i];
                printf("%d %.2f %.2f %.2f %d\n", i, vert.x, vert.y, vert.z, vert.inc_edge_ID);
        }
        printf("\n");
        printf("Faces\n");
        for(int i = 0; i < faceArray.size; i++){
                HE_Face face = faceArray.array[i];
                printf("%d %.2d\n", i, face.edge_ID);
        }
        printf("\n");
        printf("Edges\n");
        for(int i = 0; i < edgeArray.size; i++){
                HE_HalfEdge edge = edgeArray.array[i];
                printf("%.2d %.2d %.2d %.2d %.2d %.2d\n", i, edge.origin_vertex_ID+1, edge.twin_edge_ID, edge.inc_face_ID, edge.nextEdge_ID, edge.prvsEdge_ID);
        }
        printf("\n");
        }
        return;
}


HE_Object HE_load(const char* filename){
        int debug_counter = 0;
        OBJ object;
        read_obj(filename, &object);
        HE_Object ret;

        HE_load_vertices(object, &ret);
        HE_center_model(&ret);
        HE_generate_edge_and_face_array(object, &ret);

        HE_Vertex_Array verArray = ret.vertex_array;
        HE_Edge_Array edgeArray  = ret.edge_array;
        HE_Face_Array faceArray  = ret.face_array;
        free_obj(object);

          // PRINTS
        if(1){
        printf("Output from function HE_load\n");
        printf("Vertices\n");
        for(int i = 0; i < verArray.size; i++){
                HE_Vertex vert = verArray.array[i];
                printf("v%d: %d %.2f %.2f %.2f %d\n",i+1, i, vert.x, vert.y, vert.z, vert.inc_edge_ID);
        }
        printf("\n");
        printf("Faces\n");
        for(int i = 0; i < faceArray.size; i++){
                HE_Face face = faceArray.array[i];
                printf("f%d: %.2d\n", i, face.edge_ID);
        }
        printf("\n");
        printf("Edges\n");
        for(int i = 0; i < edgeArray.size; i++){
                HE_HalfEdge edge = edgeArray.array[i];
                printf("e%.2d: v%.2d %.2d %.2d %.2d %.2d\n", i, edge.origin_vertex_ID+1, edge.twin_edge_ID, edge.inc_face_ID, edge.nextEdge_ID, edge.prvsEdge_ID);
        }
        printf("\n");
        }

        return ret;
}

#endif
