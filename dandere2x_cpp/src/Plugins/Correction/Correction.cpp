//
// Created by https://github.com/CardinalPanda
//
//Licensed under the GNU General Public License Version 3 (GNU GPL v3),
//    available at: https://www.gnu.org/licenses/gpl-3.0.txt

#include "Correction.h"


/**
 * See 'PFrame' for documentation.
 * The two behave almost identical, except block matching is done on the original image, rather than
 * the previous frame.
 */

Correction::Correction(std::shared_ptr<Image> image1_predicted, std::shared_ptr<Image> image1_true, std::shared_ptr<Image> image1_compressed,
                        unsigned int block_size, std::string correction_file, int step_size) {

    this->image1_predicted = image1_predicted;
    this->image1_true = image1_true;
    this->image1_compressed = image1_compressed;
    this->step_size = step_size;
    this->max_checks = 8; //prevent diamond search from going on forever. Keep this number low for corrections for speed
    this->block_size = block_size;
    this->width = image1_predicted->width;
    this->height = image1_predicted->height;
    this->correction_file = correction_file;

    //preform checks to ensure given information is valid
    if (image1_predicted->height != image1_true->height || image1_predicted->width != image1_true->width)
        throw std::invalid_argument("PDifference image resolution does not match!");

}

void Correction::run() {
    match_all_blocks();
    draw_over();
}

void Correction::save() {
    std::ofstream out(this->correction_file + ".temp");

    for (int iter = 0; iter < blocks.size(); iter++) {

        if (blocks[iter].x_start != blocks[iter].y_start && blocks[iter].y_start != blocks[iter].y_end) {
            out << blocks[iter].x_start << "\n" <<
                   blocks[iter].y_start << "\n" <<
                   blocks[iter].x_end << "\n" <<
                   blocks[iter].y_end << std::endl;
        }
    }
    out.close();
    std::rename((this->correction_file + ".temp").c_str(), this->correction_file.c_str());

}

void Correction::draw_over() {
    for (int iter = 0; iter < blocks.size(); iter++) {
        for (int x = 0; x < block_size; x++) {
            for (int y = 0; y < block_size; y++) {
                image1_predicted->set_color(x + blocks[iter].x_start,
                                       y + blocks[iter].y_start,
                                       image1_predicted->get_color(x + blocks[iter].x_end, y + blocks[iter].y_end));
            }
        }
    }
}

void Correction::match_all_blocks() {
    for (int x = 0; x < width / block_size; x++) {
        for (int y = 0; y < height / block_size; y++) {
            match_block(x, y);
        }
    }
}

void Correction::match_block(int x, int y) {

    double min_mse = ImageUtils::mse(*image1_true, *image1_compressed, x * block_size, y * block_size,
                                     x * block_size, y * block_size, block_size);

    double sum = ImageUtils::mse(*image1_predicted, *image1_true, x * block_size, y * block_size,
                                 x * block_size, y * block_size, block_size);

    //this is producing very flickery results.
    //Perhaps resorting to lower res or adding a bias to fix.
    if (sum >= min_mse*min_mse) {
        //if it is lower, try running a diamond search around that area. If it's low enough add it as a displacement block.
        Block result = DiamondSearch::diamond_search_iterative_super(*image1_true,*image1_predicted,
                                                                     x * block_size, y * block_size,
                                                                     x * block_size, y * block_size,
                                                                     min_mse*min_mse, block_size, step_size, max_checks);
        if (result.sum <=  min_mse*min_mse)
            blocks.push_back(result);

    }
}


