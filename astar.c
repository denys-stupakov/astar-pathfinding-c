#include <stdio.h>
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

// COLORS
#define COLOR_WHITE 255, 255, 255, 255
#define COLOR_BLACK 0, 0, 0, 255
#define COLOR_RED 245, 61, 64, 255
#define COLOR_GREEN 100, 245, 71, 255
#define COLOR_BLUE 74, 231, 240, 255
#define COLOR_ORANGE 250, 185, 72, 255
#define COLOR_PURPLE 66, 135, 245, 255

// CONSTANTS
#define SCREEN_WIDTH 2400
#define SCREEN_HEIGHT 1200
#define SQUARE 25
#define ROWS SCREEN_HEIGHT / SQUARE
#define COLUMNS SCREEN_WIDTH / SQUARE
#define FPS 20
#define OPEN_LIST_CAPACITY ROWS * COLUMNS

enum Color {
	white,
	black,
	red,
	green,
	blue,
	orange,
	purple
};

struct Vertex {
	float x;
	float y;
	int row;
	int column;
	struct Vertex **neighbours;
	struct Vertex *from;
	enum Color color;
	int g_cost;
	int h_cost;
	int f_cost;
};

void draw_grid(SDL_Renderer *ren) {
	SDL_SetRenderDrawColor(ren, COLOR_BLACK);
	for(int row = 1; row < ROWS; row++) {
		SDL_RenderLine(ren, 0, row * SQUARE, SCREEN_WIDTH, row * SQUARE);
	}
	for(int column = 1; column < COLUMNS; column++) {
		SDL_RenderLine(ren, column * SQUARE, 0, column * SQUARE, SCREEN_HEIGHT);
	}
	SDL_SetRenderDrawColor(ren, COLOR_WHITE);
}

void draw_vertices(SDL_Renderer *ren, struct Vertex **vertices) {
	enum Color current_vertex_color;
	for(int row = 0; row < ROWS; row++) {
		for(int column = 0; column < COLUMNS; column++) {
			struct Vertex current_vertex = vertices[row][column];
			current_vertex_color = current_vertex.color;
			SDL_FRect v = {current_vertex.x, current_vertex.y, SQUARE, SQUARE};
			switch(current_vertex_color) {
				case black:
					SDL_SetRenderDrawColor(ren, COLOR_BLACK);
					SDL_RenderFillRect(ren, &v);
					break;
				case red:
					SDL_SetRenderDrawColor(ren, COLOR_RED);
					SDL_RenderFillRect(ren, &v);
					break;
				case green:
					SDL_SetRenderDrawColor(ren, COLOR_GREEN);
					SDL_RenderFillRect(ren, &v);
					break;
				case blue:
					SDL_SetRenderDrawColor(ren, COLOR_BLUE);
					SDL_RenderFillRect(ren, &v);
					break;
				case orange:
					SDL_SetRenderDrawColor(ren, COLOR_ORANGE);
					SDL_RenderFillRect(ren, &v);
					break;
				case purple:
					SDL_SetRenderDrawColor(ren, COLOR_PURPLE);
					SDL_RenderFillRect(ren, &v);
					break;
				default:
					break;
			}
		}
	}
	SDL_SetRenderDrawColor(ren, COLOR_WHITE);
}

float euclidian_distance(struct Vertex *v1, struct Vertex *v2) {
	return sqrt(pow(v2->x - v1->x, 2) + pow(v2->y - v1->y, 2));
}

struct Vertex **initialize_vertices() {
	struct Vertex **vertices = calloc(ROWS, sizeof(struct Vertex *));
	if(vertices == NULL) return NULL;
	for(int row = 0; row < ROWS; row++) {
		vertices[row] = calloc(COLUMNS, sizeof(struct Vertex));
		if(vertices[row] == NULL) return NULL;
	}
	for(int row = 0; row < ROWS; row++) {
		for(int column = 0; column < COLUMNS; column++) {
			struct Vertex *v = &vertices[row][column];
            v->row = row;
            v->column = column;
            v->x = column * SQUARE;
            v->y = row * SQUARE;
            v->from = NULL;
			v->color = white;
			v->h_cost = INT_MAX;
			v->g_cost = INT_MAX;
			v->f_cost = INT_MAX;
			v->neighbours = calloc(4, sizeof(struct Vertex *));
		}
	}
	return vertices;
}

void free_vertices(struct Vertex ***vertices) {
	for(int row = 0; row < ROWS; row++) {
		free((*vertices)[row]->neighbours);
		free((*vertices)[row]);
	}
	free(*vertices);
	*vertices = NULL;
}

int exists(int row, int column) {
	return row >= 0 && row < ROWS && column >= 0 && column < COLUMNS;
}

void update_neighbours(struct Vertex **vertices) {
	for(int row = 0; row < ROWS; row++) {
		for(int column = 0; column < COLUMNS; column++) {
			if(vertices[row][column].color == black) continue;
			vertices[row][column].neighbours[0] = (exists(row - 1, column) && vertices[row - 1][column].color != black) ? &vertices[row - 1][column] : NULL;
			vertices[row][column].neighbours[1] = (exists(row, column - 1) && vertices[row][column - 1].color != black) ? &vertices[row][column - 1] : NULL;
			vertices[row][column].neighbours[2] = (exists(row, column + 1) && vertices[row][column + 1].color != black) ? &vertices[row][column + 1] : NULL;
			vertices[row][column].neighbours[3] = (exists(row + 1, column) && vertices[row + 1][column].color != black) ? &vertices[row + 1][column] : NULL;
		}
	}
}

struct Vertex *popFromOpenedList(struct Vertex **opened_list, int *size_opened_list) {
	if (*size_opened_list == 0) return NULL;
	int minIndex = 0;
	for (int i = 1; i < *size_opened_list; i++) {
		if (opened_list[i]->f_cost < opened_list[minIndex]->f_cost) {
			minIndex = i;
		}
	}
	struct Vertex *vertex = opened_list[minIndex];
	opened_list[minIndex] = opened_list[*size_opened_list - 1];
	(*size_opened_list)--;
	return vertex;
}

int in_list(struct Vertex **list, struct Vertex *needle, int size_list) {
	for (int i = 0; i < size_list; i++) {
		if (list[i] == needle) return i;
	}
	return -1;
}

void reconstruct_path(struct Vertex *end_vertex) {
	struct Vertex *current_vertex = end_vertex;
	while (current_vertex != NULL && current_vertex->from != NULL) {
		current_vertex->color = red;
		current_vertex = current_vertex->from;
	}
}

struct Vertex *algorithm(struct Vertex *start_vertex, struct Vertex *end_vertex, SDL_Renderer *ren, struct Vertex **vertices) {
	if(start_vertex == NULL || end_vertex == NULL) return NULL;

	struct Vertex **opened_list = calloc(ROWS * COLUMNS, sizeof(struct Vertex *));
	struct Vertex **closed_list = calloc(ROWS * COLUMNS, sizeof(struct Vertex *));
	int size_closed_list = 0;
	int size_opened_list = 0;

	start_vertex->g_cost = 0; // Váha od štartu po tento vrchol je 0
	start_vertex->h_cost = euclidian_distance(start_vertex, end_vertex); // Odhad váhy do cieľa
	start_vertex->f_cost = start_vertex->g_cost + start_vertex->h_cost; // Celková váha

	opened_list[size_opened_list++] = start_vertex;
	SDL_FRect fillRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
	struct Vertex *current_vertex = NULL;

	while (current_vertex != end_vertex) {
		current_vertex = popFromOpenedList(opened_list, &size_opened_list); // Vyberiem vrchol s najmenšou celkovou váhou
		if (current_vertex == NULL) break;
		if (current_vertex == end_vertex) return current_vertex; // Dosiahol som cieľ

		for (int i = 0; i < 4; i++) {
			struct Vertex *neighbour = current_vertex->neighbours[i];
			if (neighbour == NULL) continue;

			int index_in_opened = in_list(opened_list, neighbour, size_opened_list);
			if(neighbour->g_cost < current_vertex->g_cost + 1) continue; // Už som tam prišiel lacnejšie

			neighbour->from = current_vertex; // Zapamätám si, odkiaľ som prišiel
			if(neighbour->color != red && neighbour != end_vertex) neighbour->color = green;

			neighbour->g_cost = current_vertex->g_cost + 1; // Zvýšim váhu o 1
			neighbour->h_cost = euclidian_distance(neighbour, end_vertex); // Prepočítam heuristiku
			neighbour->f_cost = neighbour->g_cost + neighbour->h_cost; // Spočítam celkovú váhu

			if(in_list(closed_list, neighbour, size_closed_list) == -1) {
				if(index_in_opened != -1) {
					opened_list[index_in_opened] = neighbour;
				} else {
					opened_list[size_opened_list++] = neighbour;
				}
			}
		}

		if(current_vertex != start_vertex && current_vertex != end_vertex) current_vertex->color = red;
		closed_list[size_closed_list++] = current_vertex;

		SDL_RenderClear(ren);
		SDL_SetRenderDrawColor(ren, COLOR_WHITE);
		SDL_RenderFillRect(ren, &fillRect);
		draw_vertices(ren, vertices);
		draw_grid(ren);
		SDL_RenderPresent(ren);
		SDL_Delay(1000 / FPS);
	}

	free(opened_list);
	free(closed_list);
	return current_vertex;
}
