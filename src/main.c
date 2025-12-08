#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h> // for wait time

#include "half_edge.h"
#include "matrix_math.h"
#include "obj_loader.h"

// My defines
#define SCREEN_WIDTH   800
#define SCREEN_HEIGHT  600
const float ASPECT_RATIO = (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT;
#define TRUE  1
#define FALSE 0
#define TARGET_FPS 60
#define FRAME_TARGET_TIME 1000/TARGET_FPS
#define FOV 70

int rotate = TRUE;
float rotate_speed = -1.0f/30; // frequency
vec3 translation = {0.0f, 0.0f, 0.0f};

SDL_Window*   glWindow = NULL;
SDL_Renderer* renderer = NULL;

float yaw = -90.0f;
float pitch = 0.0f;

HE_Object current_object;
char* opened_file = NULL;
int last_frame_time = 0;
int lastTime = 0;
struct nk_context *ctx;

int option_selected = 0;
int step_draw = TRUE;
int selected_vertex = 0;
int selected_face = 0;
int selected_edge = 0;

typedef enum{
        BRESENHAM,
        WU
}LineMethods;

LineMethods lineChoice;

void swapFloat(float* x1 , float* x2){
        float aux = *x1;
        *x1 = *x2;
        *x2 = aux;
}

typedef struct Color_RGBA{
        float R;
        float G;
        float B;
        float A;
}Color_RGBA;

vec3 camera_pos   = {0.0f, 0.0f,  7.0f};
vec3 camera_front = {0.0f, 0.0f, -1.0f};
vec3 camera_up    = {0.0f, 1.0f,  0.0f};
Color_RGBA red_color   = {0.85f, 0.02f, 0.12f, 0.5f};
Color_RGBA green_color = {0.20f, 1.0f, 0.69f, 0.5f};

void drawPoint(const float x,const float y){
        SDL_RenderDrawPoint(renderer, x, y);
}

void normalizePoint(vec3 v){
        v[0] = (((v[0]/SCREEN_WIDTH)*2) - 1) * ASPECT_RATIO;
        v[1] = (((v[1]/SCREEN_HEIGHT)*2) - 1);
}
void denormalizePoint(vec3 v){
        v[0] = (((v[0]/ASPECT_RATIO)+1)/2)*SCREEN_WIDTH;
        v[1] = ((v[1]+1)/2)*SCREEN_HEIGHT;
}


void drawLineH(float x1, float x2, float y1, float y2){
        if (x1 > x2){
                swapFloat(&x1, &x2);
                swapFloat(&y1, &y2);
        }

        float dx = x2-x1;  // dx
        float dy = y2-y1;  // dy
        
        int dir = 1;
        if(dy < 0) dir = -1;
        dy *= dir;

        if( dx == 0 ) return;
        float y = y1;
        float p = 2*dy - dx;

        for (int i = 0; i < dx+1 ; i++) {
                vec3 point = {x1 + i, y, 1.0f};
                normalizePoint(point);
                mat4 model;
                mm_mat4_identity(model);
                //mm_scale(model, (vec3){1.0/3,1.0/3,1.0/3});
                mm_mat4_mulv3(model, point, point);
                denormalizePoint(point);
                drawPoint(point[0], point[1]);

                if(p >= 0){
                        y += dir;
                        p = p - 2*dx;
                }
                p = p + 2*dy;
        }
}
void drawLineV(float x1, float x2, float y1, float y2){
        if (y1 > y2){
                swapFloat(&x1, &x2);
                swapFloat(&y1, &y2);
        }

        float dx = x2-x1;  // dx
        float dy = y2-y1;  // dy
        
        int dir = 1;
        if(dx < 0) dir = -1;
        dx *= dir;

        if( dy == 0 ) return;
        float x = x1;
        float p = 2*dx - dy;

        for (int i = 0; i < dy+1 ; i++) {
                vec3 point = {x, y1 + i, 1.0f};
                normalizePoint(point);
                mat4 model;
                mm_mat4_identity(model);
                //mm_scale(model, (vec3){1.0/3,1.0/3,1.0/3});
                mm_mat4_mulv3(model, point, point);
                denormalizePoint(point);
                drawPoint(point[0], point[1]);

                if(p >= 0){
                        x += dir;
                        p = p - 2*dy;
                }
                p = p + 2*dx;
        }
}
void drawLineBresenham(float x1, float y1, float z1, float x2, float y2, float z2, const unsigned int resolution, const float point_size_multiplier, const Color_RGBA color){
        SDL_SetRenderDrawColor(renderer, color.R*255, color.G*255, color.B*255, color.A*255);
        x1 = (((x1/ASPECT_RATIO)+1)/2)*SCREEN_WIDTH;
        x2 = (((x2/ASPECT_RATIO)+1)/2)*SCREEN_WIDTH;
        y1 = ((y1+1)/2)*SCREEN_HEIGHT;
        y2 = ((y2+1)/2)*SCREEN_HEIGHT;

        if(fabs(x2-x1) > fabs(y2-y1))
                drawLineH(x1, x2, y1, y2);
        else
                drawLineV(x1, x2, y1, y2);


        /**
        mat4 model;
        mm_mat4_identity(model);
        if(rotate == FALSE){}
                mm_rotate(model, rotate_speed*((float)SDL_GetTicks()/1000.0f)*MM_PI*2, (vec3){0.0f, 1.0f, 0.0f});

        mat4 view;   // Camera space
        mm_mat4_identity(view);

        // camera pos global
        vec3 target_dir;
        vec3 direction; // mouse dir
        direction[0] = cos(mm_rad(yaw)) * cos(mm_rad(pitch));
        direction[1] = sin(mm_rad(pitch));
        direction[2] = sin(mm_rad(yaw)) * cos(mm_rad(pitch));
        mm_normalize_to(direction, camera_front); // mouse dir is normalized to camera front
        mm_vec3_add(camera_pos, camera_front, target_dir);
        mm_lookat(camera_pos, target_dir, camera_up, view);

        /////////////////////////////////////////////////////////////////////
        mat4 proj;  // Clip space
        mm_mat4_identity(proj);
        mm_perspective(mm_rad(FOV), (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 0.1f, 100.0f, proj);
        **/
}

void drawLineXiaolinWu(float x1, float y1, float z1, float x2, float y2, float z2, const unsigned int resolution, const float point_size_multiplier, const Color_RGBA color){
        vec3 point1 = {x1, y1, z1};
        vec3 point2 = {x2, y2, z2};
        mat4 model;
        mm_mat4_identity(model);
        //mm_rotate(model, rotate_speed*((float)SDL_GetTicks()/1000.0f)*MM_PI*2, (vec3){0.0f, 1.0f, 0.0f});
        //mm_translate(model, (vec3){0.25f,0.0f,0.0f});
        //mm_scale(model, (vec3){1.0/3,1.0/3,1.0/3});
        mm_mat4_mulv3(model, point1, point1);
        mm_mat4_mulv3(model, point2, point2);
        x1 = point1[0];
        y1 = point1[1];
        x2 = point2[0];
        y2 = point2[1];

        SDL_SetRenderDrawColor(renderer, color.R*255, color.G*255, color.B*255, color.A*255);
        x1 = (((x1/ASPECT_RATIO)+1)/2)*SCREEN_WIDTH;
        x2 = (((x2/ASPECT_RATIO)+1)/2)*SCREEN_WIDTH;
        y1 = ((y1+1)/2)*SCREEN_HEIGHT;
        y2 = ((y2+1)/2)*SCREEN_HEIGHT;


        // Actual method ==============================================================
        if(fabs(y2-y1) < fabs(x2-x1)){
                if (x2 < x1){
                        swapFloat(&x1, &x2);
                        swapFloat(&y1, &y2);
                }
                float dx = x2 - x1;
                float dy = y2 - y1;
                float m = 1;
                if(dx != 0) m = dy/dx;

                float overlap = 1 - ((x1 + 0.5) - (int)(x1 + 0.5));
                float distStart = y1 - (int)y1;
                SDL_SetRenderDrawColor(renderer, color.R*255, color.G*255, color.B*255, color.A*255*((1-distStart)*overlap));
                drawPoint((int)(x1+0.5), (int)y1);
                SDL_SetRenderDrawColor(renderer, color.R*255, color.G*255, color.B*255, color.A*255*(distStart*overlap));
                drawPoint((int)(x1+0.5), (int)y1 + 1);

                overlap = ((x2 - 0.5) - (int)(x2 - 0.5));
                float distEnd = y2 - (int)y2;
                SDL_SetRenderDrawColor(renderer, color.R*255, color.G*255, color.B*255, color.A*255*((1-distEnd)*overlap));
                drawPoint((int)(x2+0.5), (int)y2);
                SDL_SetRenderDrawColor(renderer, color.R*255, color.G*255, color.B*255, color.A*255*(distEnd*overlap));
                drawPoint((int)(x2+0.5), (int)y2 + 1);

                for(int i = 1; i <= (int)dx + 0.5; i++){
                        float x = x1 + i;
                        float y = y1 + i * m;
                        int ix = (int)x;
                        int iy = (int)y;
                        float dist = y - iy;
                        SDL_SetRenderDrawColor(renderer, color.R*255, color.G*255, color.B*255, color.A*255*(1-dist));
                        drawPoint(ix, iy);
                        SDL_SetRenderDrawColor(renderer, color.R*255, color.G*255, color.B*255, color.A*255*(dist));
                        drawPoint(ix, iy+1);
                }
        }else{ //============================================================================
                if (y2 < y1){
                        swapFloat(&x1, &x2);
                        swapFloat(&y1, &y2);
                }
                float dx = x2 - x1;
                float dy = y2 - y1;
                float m = 1;
                if(dy != 0) m = dx/dy;

                float overlap = 1 - ((y1 + 0.5) - (int)(y1 + 0.5));
                float distStart = y1 - (int)y1;
                SDL_SetRenderDrawColor(renderer, color.R*255, color.G*255, color.B*255, color.A*255*((1-distStart)*overlap));
                drawPoint((int)(x1+0.5), (int)y1);
                SDL_SetRenderDrawColor(renderer, color.R*255, color.G*255, color.B*255, color.A*255*(distStart*overlap));
                drawPoint((int)(x1+0.5), (int)y1 + 1);

                overlap = ((y2 - 0.5) - (int)(y2 - 0.5));
                float distEnd = y2 - (int)y2;
                SDL_SetRenderDrawColor(renderer, color.R*255, color.G*255, color.B*255, color.A*255*((1-distEnd)*overlap));
                drawPoint((int)(x2+0.5), (int)y2);
                SDL_SetRenderDrawColor(renderer, color.R*255, color.G*255, color.B*255, color.A*255*(distEnd*overlap));
                drawPoint((int)(x2+0.5), (int)y2 + 1);

                for(int i = 1; i <= (int)dy + 0.5; i++){
                        float x = x1 + i * m;
                        float y = y1 + i;
                        int ix = (int)x;
                        int iy = (int)y;
                        float dist = x - ix;
                        SDL_SetRenderDrawColor(renderer, color.R*255, color.G*255, color.B*255, color.A*255*(1-dist));
                        drawPoint(ix, iy);
                        SDL_SetRenderDrawColor(renderer, color.R*255, color.G*255, color.B*255, color.A*255*(dist));
                        drawPoint(ix+1, iy);
                }
        }

}

float* HE_get_object_verts_as_array(HE_Object object){
        HE_Vertex_Array verts = object.vertex_array;
        float* output = (float*)malloc(sizeof(float)*3*verts.size);
        for(int i = 0; i < verts.size; i++){
                output[i*3]       = verts.array[i].x;
                output[(i*3) + 1] = verts.array[i].y;
                output[(i*3) + 2] = verts.array[i].z;
        }
        return output;
}

void update_itemlist(char*** orig, unsigned int* size){
        char** items = *orig;
        for(int i = 0; i < *size; i++)
                free(items[i]);
        free(items);

        *size = current_object.vertex_array.size;
        items = (char**)malloc(sizeof(char*)*(*size));
        for(int i = 0; i < *size; i++){
                char* str_buff = (char*)malloc(sizeof(char)*50);
                snprintf(str_buff,sizeof(str_buff), "%s%d", "v", i+1);
                //printf("%s ", str_buff);
                items[i] = str_buff;
        }
        *orig = items;
}

void update_facelist(char***orig, unsigned int* size){
        char** items = *orig;
        for(int i = 0; i < *size; i++)
                free(items[i]);
        free(items);

        *size = current_object.face_array.size;
        items = (char**)malloc(sizeof(char*)*(*size));
        for(int i = 0; i < *size; i++){
                char* str_buff = (char*)malloc(sizeof(char)*50);
                snprintf(str_buff,sizeof(str_buff), "%s%d", "f", i);
                //printf("%s ", str_buff);
                items[i] = str_buff;
        }
        *orig = items;
}

void update_edgelist(char***orig, unsigned int* size){
        char** items = *orig;
        for(int i = 0; i < *size; i++)
                free(items[i]);
        free(items);

        *size = current_object.edge_array.size;
        items = (char**)malloc(sizeof(char*)*(*size));
        for(int i = 0; i < *size; i++){
                char* str_buff = (char*)malloc(sizeof(char)*50);
                snprintf(str_buff,sizeof(str_buff), "%s%d", "e", i);
                items[i] = str_buff;
        }
        *orig = items;
}


int update_clipped = TRUE;
HE_Object clipped;

// TODO: Ajustar clipping de acordo com resolução da tela (dica: ajustar lim)
int get_clip_code(const float x, const float y, const float lim){
        int code = 0;
        if(x < -lim)
                code += 1;
        if(x > lim)
                code += 2;
        if(y < -lim)
                code += 4;
        if(y > lim)
                code += 8;
        return code;
}

int clip_line(float* x1, float* y1, float* x2, float* y2, const float lim){
        int code1 = get_clip_code(*x1, *y1, lim);
        int code2 = get_clip_code(*x2, *y2, lim);
        if(code1 == 0 && code2 == 0){
                //printf("Line fully inside\n");
                return 2;
        }
        int logAND = code1 & code2;
        if(logAND != 0){
                //printf("Line fully outside\n");
                return 0;
        }
        //printf("(%.2f, %.2f) -> (%.2f, %.2f): %d, %d\n", *x1, *y1, *x2, *y2, code1, code2);
        //printf("Line partially inside\n");
        if(code1 != 0){
                //printf("Start prunning p1\n");
                float x = *x1;
                float y = *y1;
                //printf("Start: (%.2f, %.2f)\n", x, y);
                while(get_clip_code(x,y, lim) != 0){
                        float u = 0.01;
                        x = x + u*(*x2 - x);
                        y = y + u*(*y2 - y);
                        u *= 2;
                }
                *x1 = x;
                *y1 = y;
                //printf("After: (%.2f, %.2f)\n\n", *x1, *y1);
        }
        if(code2 != 0){
                //printf("Start prunning p2\n");
                float x = *x2;
                float y = *y2;
               //printf("Start: (%.2f, %.2f)\n", x, y);
                while(get_clip_code(x,y, lim) != 0){
                        float u = 0.01;
                        x = x + u*(*x1 - x);
                        y = y + u*(*y1 - y);
                        u *= 2;
                }
                *x2 = x;
                *y2 = y;
                //printf("After: (%.2f, %.2f)\n\n", *x2, *y2);
        }
        return 1; 
}

HE_Object HE_clip(const HE_Object source){
        HE_Object clipped;

        HE_Edge_Array   edgeArray = source.edge_array;
        HE_Vertex_Array verArray  = source.vertex_array;
        HE_Face_Array   faceArray = source.face_array;

        HE_Edge_Array   newEdgeArray = {NULL,0};
        HE_Vertex_Array newVertArray = {NULL,0};
        HE_Face_Array   newFaceArray = {NULL,0};

        for(int f = 0; f < faceArray.size; f++){
                puts("Creating new face");
                HE_faceArray_Push(&newFaceArray, -1);

                int startEdge = faceArray.array[f].edge_ID;
                int currEdge = startEdge;
                int originVertex = edgeArray.array[currEdge].origin_vertex_ID;
                float x1 = verArray.array[originVertex].x;
                float y1 = verArray.array[originVertex].y;
                float z1 = verArray.array[originVertex].z;

                int nextEdge = edgeArray.array[startEdge].nextEdge_ID;
                int nextVertex = edgeArray.array[nextEdge].origin_vertex_ID;
                float x2 = verArray.array[nextVertex].x;
                float y2 = verArray.array[nextVertex].y;
                float z2 = verArray.array[nextVertex].z;

                int newStartingEdgeID = newEdgeArray.size;
                //int newStartingVertID = verArray.size;
                do{
                        printf("Input: v%d(%.2f %.2f) -> v%d(%.2f %.2f)\n",originVertex+1, x1, y1, nextVertex+1, x2, y2);
                        int res = clip_line(&x1, &y1, &x2, &y2, 0.9);
                        if(res == 0){ // If line is fully outside
                                puts("Discarded");
                                currEdge = nextEdge;
                                originVertex = edgeArray.array[currEdge].origin_vertex_ID;
                                x1 = verArray.array[originVertex].x;
                                y1 = verArray.array[originVertex].y;
                                nextEdge = edgeArray.array[nextEdge].nextEdge_ID;
                                nextVertex = edgeArray.array[nextEdge].origin_vertex_ID;
                                x2 = verArray.array[nextVertex].x;
                                y2 = verArray.array[nextVertex].y;
                                continue;
                        }
                        HE_vertexArray_Push(&newVertArray, x1, y1, z1, -1);
                        printf("v%d: (%.2f %.2f %.2f) %d\n", newVertArray.size, x1, y1, z1, newVertArray.size);
                        HE_vertexArray_Push(&newVertArray, x2, y2, z2, -1);
                        printf("v%d: (%.2f %.2f %.2f) %d\n", newVertArray.size, x2, y2, z2, -1);
                        HE_edgeArray_Push(&newEdgeArray, newVertArray.size-2, -1, newFaceArray.size-1, newEdgeArray.size+1, -1);
                        printf("e%d: v%d %.2d %.2d e%d e%d\n", newEdgeArray.size-1, newVertArray.size-1, -1, newFaceArray.size-1, newEdgeArray.size, -1);
                        
                        currEdge = nextEdge;
                        originVertex = edgeArray.array[currEdge].origin_vertex_ID;
                        x1 = verArray.array[originVertex].x;
                        y1 = verArray.array[originVertex].y;
                        nextEdge = edgeArray.array[nextEdge].nextEdge_ID;
                        nextVertex = edgeArray.array[nextEdge].origin_vertex_ID;
                        x2 = verArray.array[nextVertex].x;
                        y2 = verArray.array[nextVertex].y;
                }while(currEdge != startEdge);

                if(newEdgeArray.size > 0 && newEdgeArray.size != newStartingEdgeID){
                        puts("Closing shape");
                        HE_edgeArray_Push(&newEdgeArray, newVertArray.size-1, -1, newFaceArray.size-1, newEdgeArray.size+1, -1);
                        HE_vertexArray_Push(&newVertArray, -0.9, 0.9, z1, -1);  // TODO: make this modular

                        HE_edgeArray_Push(&newEdgeArray, newVertArray.size-1, -1, newFaceArray.size-1, newStartingEdgeID, -1);
                        //newEdgeArray.array[newEdgeArray.size-1].nextEdge_ID = newStartingEdgeID;
                }
                puts("Face created");
                puts("");
        }
        puts("Vertices");
        for(int i = 0; i < newVertArray.size; i++){
                HE_Vertex vert = newVertArray.array[i];
                printf("v%d (%.2f %.2f %.2f) %d\n", i+1, vert.x, vert.y, vert.z, vert.inc_edge_ID);
        }
        puts("");
        printf("Faces\n");
        for(int i = 0; i < newFaceArray.size; i++){
                HE_Face face = newFaceArray.array[i];
                printf("%d %.2d\n", i, face.edge_ID);
        }
        puts("");
        printf("Edges\n");
        for(int i = 0; i < newEdgeArray.size; i++){
                HE_HalfEdge edge = newEdgeArray.array[i];
                printf("e%d: v%d %.2d %.2d ne%d pe%d\n", i, edge.origin_vertex_ID+1, edge.twin_edge_ID, edge.inc_face_ID, edge.nextEdge_ID, edge.prvsEdge_ID);
        }
        puts("");
        /**
        for(int e = 0; e < edgeArray.size; e++){
                int originVertex = edgeArray.array[e].origin_vertex_ID;
                float x1 = verArray.array[originVertex].x;
                float y1 = verArray.array[originVertex].y;
                float z1 = verArray.array[originVertex].z;

                int nextEdge = edgeArray.array[e].nextEdge_ID;
                int nextVertex = edgeArray.array[nextEdge].origin_vertex_ID;
                float x2 = verArray.array[nextVertex].x;
                float y2 = verArray.array[nextVertex].y;
                float z2 = verArray.array[nextVertex].z;

                int res = clip_line(&x1, &y1, &x2, &y2, 0.9);
                if(res == 0) // If line is fully outside
                        continue;
                if(res == 2) // Line fully inside, no need to update
                        continue;
                
                HE_vertexArray_Push(&verArray, x1, y1, z1, nextEdge);
                int nextNextEdge = edgeArray.array[nextEdge].nextEdge_ID;
                HE_vertexArray_Push(&verArray, x2, y2, z2, nextNextEdge);


        }**/

        /**
        for(int e = 0; e < edgeArray.size; e++){
                int originVertex = edgeArray.array[e].origin_vertex_ID;
                float x1 = verArray.array[originVertex].x;
                float y1 = verArray.array[originVertex].y;
                float z1 = verArray.array[originVertex].z;

                int nextEdge = edgeArray.array[e].nextEdge_ID;
                int nextVertex = edgeArray.array[nextEdge].origin_vertex_ID;
                float x2 = verArray.array[nextVertex].x;
                float y2 = verArray.array[nextVertex].y;
                float z2 = verArray.array[nextVertex].z;

                drawLineXiaolinWu(x1, y1, z1, x2, y2, z2, 20, 0.4f, (Color_RGBA){0.85f, 0.65f, 0.12f, 0.5f});
        }
        **/
        for(int e = 0; e < newEdgeArray.size; e++){
                //printf("drawing edge: %d\n", e);
                fflush(stdout);
                int originVertex = newEdgeArray.array[e].origin_vertex_ID;
                float x1 = newVertArray.array[originVertex].x;
                float y1 = newVertArray.array[originVertex].y;
                //printf("v%d: ( %.2f %.2f) -> ", originVertex+1, x1, y1);
                fflush(stdout);

                int nextEdge = newEdgeArray.array[e].nextEdge_ID;
                int nextVertex = newEdgeArray.array[nextEdge].origin_vertex_ID;
                float x2 = newVertArray.array[nextVertex].x;
                float y2 = newVertArray.array[nextVertex].y;
               // printf("v%d: ( %.2f %.2f)\n\n", nextVertex+1, x2, y2);
                fflush(stdout);

                drawLineXiaolinWu(x1, y1, 0, x2, y2, 0, 20, 0.4f, (Color_RGBA){0.85f, 0.65f, 0.12f, 0.5f});
        }

        clipped.edge_array = newEdgeArray;
        clipped.vertex_array = newVertArray;
        clipped.face_array = newFaceArray;
        return clipped;
}

void HE_draw(const HE_Object object){
        if(update_clipped == TRUE){
                clipped = HE_clip(object);
                //update_clipped = FALSE;
        }
        /**
        HE_Edge_Array   edgeArray = clipped.edge_array;
        HE_Vertex_Array verArray  = clipped.vertex_array;
        HE_Face_Array   faceArray = clipped.face_array;

        float scds = 8;  // Time to draw whole figure
        float cnt = (int)(SDL_GetTicks()/((scds*1000.0f)/edgeArray.size))%(edgeArray.size) + 1;
        int step = 0;

        step_draw = FALSE;
        if(step_draw == FALSE)
                cnt = edgeArray.size;
        for(int e = 0; e <= edgeArray.size; e++){
                if(step == cnt)
                        break;
                int originVertex = edgeArray.array[e].origin_vertex_ID;
                float x1 = verArray.array[originVertex].x;
                float y1 = verArray.array[originVertex].y;
                float z1 = verArray.array[originVertex].z;

                int nextEdge = edgeArray.array[e].nextEdge_ID;
                int nextVertex = edgeArray.array[nextEdge].origin_vertex_ID;
                float x2 = verArray.array[nextVertex].x;
                float y2 = verArray.array[nextVertex].y;
                float z2 = verArray.array[nextVertex].z;

                if (step_draw == FALSE && (originVertex == selected_vertex || nextVertex == selected_vertex) && option_selected == 3){
                        if(lineChoice == BRESENHAM)
                                drawLineBresenham(x1, y1, z1, x2, y2, z2, 20, 0.4f, (Color_RGBA){0.85f, 0.02f, 0.12f, 1.0f});
                        if(lineChoice == WU)
                                drawLineXiaolinWu(x1, y1, z1, x2, y2, z2, 20, 0.4f, (Color_RGBA){0.85f, 0.12f, 0.12f, 1.0f});
                }else if (step == cnt-1 && step_draw == TRUE){
                        if(lineChoice == BRESENHAM)
                                drawLineBresenham(x1, y1, z1, x2, y2, z2, 20, 0.4f, (Color_RGBA){0.85f, 0.02f, 0.12f, 1.0f});
                        if(lineChoice == WU)
                                drawLineXiaolinWu(x1, y1, z1, x2, y2, z2, 20, 0.4f, (Color_RGBA){0.85f, 0.02f, 0.12f, 1.0f});
                }else{
                        if(lineChoice == BRESENHAM)
                                drawLineBresenham(x1, y1, z1, x2, y2, z2, 20, 0.25f, (Color_RGBA){1.0f, 0.6f, 0.133f, 1.0f});
                        if(lineChoice == WU)
                                drawLineXiaolinWu(x1, y1, z1, x2, y2, z2, 20, 0.25f, (Color_RGBA){1.0f, 0.6f, 0.133f, 0.8f});
                }
                step++;
        }
        **/
}

void init() {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        current_object = HE_load("test.obj");
}
int update_item = FALSE;

void saveObject(char* filename){
        FILE* file = fopen(filename, "w");
        
        int vertex_size = current_object.vertex_array.size;
        for(int i = 0; i < vertex_size; i++){
                float x = current_object.vertex_array.array[i].x;
                float y = current_object.vertex_array.array[i].y;
                float z = current_object.vertex_array.array[i].z;
                fprintf(file, "v %g %g %g\n", x, y, z);
        }
        int face_size = current_object.face_array.size;
        for(int i = 0; i < face_size; i++){
                int first_edge = current_object.face_array.array[i].edge_ID; 
                int vertex = current_object.edge_array.array[first_edge].origin_vertex_ID;
                fprintf(file, "f %d ", vertex);
                int next_edge = current_object.edge_array.array[first_edge].nextEdge_ID;
                while(next_edge != first_edge){
                        vertex = current_object.edge_array.array[next_edge].origin_vertex_ID;
                        fprintf(file, "%d ", vertex);
                        next_edge = current_object.edge_array.array[next_edge].nextEdge_ID;
                }
                fprintf(file, "\n");
        }
        fclose(file);
}

void transformObject(char command[50]){
        printf("Command read: %s\n", command);
        mat4 transform;
        float rad = 0;
        float t1 = 0, t2 = 0, t3 = 0;
        char c;
        char axis;
        mm_mat4_identity(transform);
        switch(command[0]){
                case 't':
                        sscanf(command, "%c %f %f", &c, &t1, &t2);
                        printf("%c %.2f %.2f\n",c, t1, t2);
                        mm_translate(transform, (vec3){t1, t2, 0});
                        break;
                case 's':
                        sscanf(command, "%c %f %f", &c, &t1, &t2);
                        mm_scale(transform, (vec3){t1, t2, 0});
                        break;
                case 'r':
                        sscanf(command, "%c %f", &c, &rad);
                        mm_rotate(transform, rad*0.0174533, (vec3){0, 0.0f, 1.0f});
                        break;
                case 'h':
                        sscanf(command, "%c %c %f", &c, &axis, &t1);
                        if (axis == 'x')
                                mm_shear_x(transform, t1);
                        if (axis == 'y')
                                mm_shear_y(transform, t1);
                        break;
                case 'm':
                        sscanf(command, "%c %c", &c, &axis);
                        if (axis == 'x')
                                mm_mirror_x(transform);
                        if (axis == 'y')
                                mm_mirror_y(transform);
                        break;
                default:
                        puts("Comando não reconhecido 2");
                        return;
        }
        HE_applyTransform(transform, current_object);
        fflush(stdout);

        saveObject("transformado.obj");
        return;
}

void input(int * quit){
        SDL_Event e;
        const float camera_speed = 0.1f;
        const Uint8* states = SDL_GetKeyboardState(NULL);
        while(SDL_PollEvent(&e)){
                switch(e.type){
                        case SDL_QUIT:
                                *quit = TRUE;
                                break;
                        case SDL_KEYDOWN:
                                if(e.key.keysym.sym == SDLK_ESCAPE)
                                        *quit = TRUE;
                                if(e.key.keysym.sym == SDLK_TAB){
                                        if(SDL_GetRelativeMouseMode() == SDL_TRUE){
                                                SDL_SetRelativeMouseMode(SDL_FALSE);
                                        }else{
                                                SDL_SetRelativeMouseMode(SDL_TRUE);
                                                SDL_GetRelativeMouseState(NULL, NULL);
                                        }
                                }
                                if(e.key.keysym.sym == SDLK_SLASH){
                                        char command[50];
                                        fgets(command, 50, stdin);
                                        command[49] = '\0';
                                        transformObject(command); 
                                }
                                break;  
                        case SDL_DROPFILE:
                                if(opened_file != NULL)
                                        free(opened_file);
                                opened_file = (char*)malloc(sizeof(char)*(strlen(e.drop.file)+1));
                                strcpy(opened_file, e.drop.file);
                                printf("File dropped: %s\n", opened_file);
                                free(current_object.vertex_array.array);
                                free(current_object.face_array.array);
                                free(current_object.edge_array.array);
                                current_object = HE_load(opened_file);
                                update_item = TRUE;
                                SDL_free(e.drop.file);
                                break;
                }
        }
        if(states[SDL_SCANCODE_W]){
                vec3 test;
                mm_vec3_copy(camera_front, test);
                test[1] = 0.0f;
                mm_vec3_muladds(test, camera_speed, camera_pos); //pos += (front*spd)
        }
        if(states[SDL_SCANCODE_S]){
                vec3 test;
                mm_vec3_copy(camera_front, test);
                test[1] = 0.0f;
                mm_vec3_muladds(test, -camera_speed, camera_pos);
        }
        if(states[SDL_SCANCODE_A]){
                vec3 aux;
                mm_vec3_crossn(camera_front, camera_up, aux);
                mm_vec3_muladds(aux, -camera_speed, camera_pos);
        }
        if(states[SDL_SCANCODE_D]){
                vec3 aux;
                mm_vec3_crossn(camera_front, camera_up, aux);
                mm_vec3_muladds(aux, camera_speed, camera_pos);
        }

        const float sensitivity = 0.25;
        int x = 0, y = 0;
        if(SDL_GetRelativeMouseMode() == SDL_TRUE)
                SDL_GetRelativeMouseState(&x, &y);
        yaw += x*sensitivity;
        pitch -= y*sensitivity;
        if(pitch > 89.9f)
                pitch = 89.9f;
        if(pitch < -89.9f)
                pitch = -89.9f;
        if(0){
        printf("%d, %d\n", x, y);
        printf("Pos: %.2f, %.2f, %.2f\n", camera_pos[0], camera_pos[1], camera_pos[2]);
        printf("Fnt: %.2f, %.2f, %.2f\n", camera_front[0], camera_front[1], camera_front[2]);
        printf("Up:  %.2f, %.2f, %.2f\n\n", camera_up[0], camera_up[1], camera_up[2]);
        }
}

void update(int* step){

        int wait_time = FRAME_TARGET_TIME - (SDL_GetTicks() - last_frame_time);
        if(wait_time > 0 && wait_time <= FRAME_TARGET_TIME)
                SDL_Delay(wait_time);

        float delta_time = (SDL_GetTicks() - last_frame_time) / 1000.0f;
        last_frame_time = SDL_GetTicks();

        *step = 0;
        float currentTime = SDL_GetTicks();
        if(currentTime - lastTime >= 500.0f){ // Updates every x seconds
                *step = 1;
                lastTime = SDL_GetTicks();
        }
}

typedef struct{
        char* string;
        unsigned int size;
} String;


void draw(){
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 100, 255, 255, 255); // White color (RGBA)
        HE_draw(current_object);
        SDL_RenderPresent(renderer);
}

int main(int argc, char** argv) {
        if(argc == 2){
                if(strcmp(argv[1], "bresenham") == 0){
                        printf("Using Bresenham\n");
                        fflush(stdout);
                        lineChoice = BRESENHAM;
                }
                if(strcmp(argv[1], "wu") == 0){
                        printf("Using Xiaolin Wu\n");
                        fflush(stdout);
                        lineChoice = WU;
                }
        }


        if(SDL_Init(SDL_INIT_EVERYTHING) != 0){
                printf("SDL2 could not initialize video subsystem\n");
                exit(1);
        }

        glWindow = SDL_CreateWindow("NO OpenGL", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if(glWindow == NULL){
                printf("Error creating window\n");
                exit(1);
        }

        renderer = SDL_CreateRenderer(glWindow, -1, SDL_RENDERER_ACCELERATED);
        if(renderer == NULL){
                printf("Error creating renderer\n");
                exit(1);
        }

        int quit = FALSE;

        init();

        char** items = NULL;
        unsigned int list_size = 0;
        update_itemlist(&items, &list_size);

        char** face_list = NULL;
        unsigned int face_list_size = 0;
        update_facelist(&face_list, &face_list_size);
        
        char** edge_list = NULL;
        unsigned int edge_list_size = 0;
        update_edgelist(&edge_list, &edge_list_size);

        while(quit == FALSE){
                input(&quit);
                if(update_item == TRUE){
                        update_item = FALSE;
                        update_itemlist(&items, &list_size);
                        update_facelist(&face_list, &face_list_size);
                        update_edgelist(&edge_list, &edge_list_size);
                        selected_vertex = 0;
                        selected_edge = 0;
                        selected_face = 0;
                }
                draw();
        }
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(glWindow);
        SDL_Quit();
        free(opened_file);
        free(current_object.vertex_array.array);
        free(current_object.face_array.array);
        free(current_object.edge_array.array);
        for(int i = 0; i < face_list_size; i++)
                free(face_list[i]);
        free(face_list);
        for(int i = 0; i < list_size; i++)
                free(items[i]);
        free(items);
        for(int i = 0; i < edge_list_size; i++)
                free(edge_list[i]);
        free(edge_list);
        return 0;
}
