
namespace nv {

    class Color32;
    struct ColorBlock;
    struct BlockDXT1;
    class Vector3;

    // All these functions return MSE.

    // Optimal compressors:
    /*float compress_dxt1_single_color_optimal(const Color32 & rgb, BlockDXT1 * output);
    float compress_dxt1_single_color_optimal(const ColorBlock & input, BlockDXT1 * output);
    float compress_dxt1_optimal(const ColorBlock & input, BlockDXT1 * output);



    // Brute force with restricted search space:
    float compress_dxt1_bounding_box_exhaustive(const ColorBlock & input, BlockDXT1 * output);
    float compress_dxt1_best_fit_line_exhaustive(const ColorBlock & input, BlockDXT1 * output);


    // Fast least squres fitting compressors:
    float compress_dxt1_least_squares_fit(const ColorBlock & input, BlockDXT1 * output);
    float compress_dxt1_least_squares_fit_iterative(const ColorBlock & input, BlockDXT1 * output);
    */

    float compress_dxt1_single_color_optimal(Color32 c, BlockDXT1 * output);
    float compress_dxt1_single_color_optimal(const Vector3 & color, BlockDXT1 * output);

    float compress_dxt1_least_squares_fit(const Vector3 input_colors[16], const Vector3 * colors, const float * weights, int count, BlockDXT1 * output);
    float compress_dxt1_bounding_box_exhaustive(const Vector3 input_colors[16], const Vector3 * colors, const float * weights, int count, int search_limit, BlockDXT1 * output);
    float compress_dxt1_cluster_fit(const Vector3 input_colors[16], const Vector3 * colors, const float * weights, int count, BlockDXT1 * output);


    float compress_dxt1(const Vector3 colors[16], const float weights[16], BlockDXT1 * output);

}