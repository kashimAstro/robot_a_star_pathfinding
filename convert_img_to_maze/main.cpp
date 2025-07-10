#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <vector>
#include <string>
#include <iostream>

enum class CellType {
    CELL_EMPTY = 0,     // corrisponde al nero (RGB 0,0,0)
    CELL_OBSTACLE = 1,  // corrisponde al bianco (RGB 255,255,255)
    CELL_START = 3,     // corrisponde al rosso (RGB 255,0,0)
    CELL_GOAL = 2       // corrisponde al verde (RGB 0,255,0)
};

std::pair< std::vector<std::vector<int>>, std::vector<std::vector<std::string>> > convert_image_file_to_2dgrid(const char* filename) {

    int width, height, channels;
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 3);
    if (data == NULL) {
        std::cerr << "errore caricamento immagine: " << filename << ", " << stbi_failure_reason() << std::endl;
        return {}; //
    }
    std::vector<std::vector<int>> grid(height, std::vector<int>(width));
    std::vector<std::vector<std::string>> sgrid(height, std::vector<std::string>(width));

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            int pixel_index = (i * width + j) * 3;

            // estrae i valori dei canali R, G, B
            unsigned char red = data[pixel_index];
            unsigned char green = data[pixel_index + 1];
            unsigned char blue = data[pixel_index + 2];

            // mappa il colore al tipo di cella corrispondente
            if (red == 0 && green == 0 && blue == 0) { // nero (RGB 0,0,0)
                grid[i][j] = static_cast<int>(CellType::CELL_EMPTY);
		sgrid[i][j] = "CELL_EMPTY";
            } else if (red == 255 && green == 255 && blue == 255) { // bianco (RGB 255,255,255)
                grid[i][j] = static_cast<int>(CellType::CELL_OBSTACLE);
		sgrid[i][j] = "CELL_OBSTACLE";
            } else if (red == 255 && green == 0 && blue == 0) { // rosso (RGB 255,0,0)
                grid[i][j] = static_cast<int>(CellType::CELL_START);
		sgrid[i][j] = "CELL_START";
            } else if (red == 0 && green == 255 && blue == 0) { // verde (RGB 0,255,0)
                grid[i][j] = static_cast<int>(CellType::CELL_GOAL);
		sgrid[i][j] = "CELL_GOAL";
            } else {
                grid[i][j] = static_cast<int>(CellType::CELL_OBSTACLE);
		sgrid[i][j] = "CELL_OBSTACLE";
            }
        }
    }
    stbi_image_free(data);

    return std::pair< std::vector<std::vector<int>>, std::vector<std::vector<std::string>> >(grid,sgrid);
}

int main(int argc, char ** argv) {
    if(argc != 2) { 
       std::cerr << "./create_map image.[jpg/png]" << std::endl;
       return 0;
    }

    const char* image_path = argv[1]; 
    std::pair< std::vector<std::vector<int>>, std::vector<std::vector<std::string>> > grid = convert_image_file_to_2dgrid(image_path);
    if (!grid.first.empty()) {
        //std::cout << "\ngriglia convertita (" << grid.first.size() << " righe x " << grid.first[0].size() << " colonne):" << std::endl;
        int display_rows = grid.first.size();
        int display_cols = grid.first[0].size();

        for (size_t i = 0; i < display_rows; ++i) {
            for (size_t j = 0; j < display_cols; ++j) {
		    std::cout << grid.first[i][j] << " ";
            }
            std::cout << std::endl;
        }
	/*
	std::cout << "================================" << std::endl;
        std::cout << "std::vector<std::vector<int>> grid_map_" + std::to_string(grid.first.size()) +"x" + std::to_string(grid.first[0].size()) + "_1 = {" ;
        for (size_t i = 0; i < display_rows; ++i) {
	    std::cout << "{" ;
            for (size_t j = 0; j < display_cols; ++j) {
		    std::cout << grid.second[i][j];
		    if(j<display_cols-1) std::cout << ",";
            }
	    std::cout << "}";
            if(i<display_rows-1) std::cout << ",";
            std::cout << std::endl;
        }
	std::cout << "};" << std::endl;*/
    } else {
         std::cout << "griglia vuota conversione fallita." << std::endl;
    }
    return 0;
}
