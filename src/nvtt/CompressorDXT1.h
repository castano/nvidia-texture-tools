
namespace nv {

    struct BlockDXT1;
    class Vector3;
    class Vector4;

    void init_dxt1();

    // All these functions return MSE.

    float compress_dxt1_single_color(const Vector3 * colors, const float * weights, int count, const Vector3 & color_weights, BlockDXT1 * output);
    //float compress_dxt1_least_squares_fit(const Vector4 input_colors[16], const Vector3 * colors, const float * weights, int count, const Vector3 & color_weights, BlockDXT1 * output);
    float compress_dxt1_bounding_box_exhaustive(const Vector4 input_colors[16], const Vector3 * colors, const float * weights, int count, const Vector3 & color_weights, bool three_color_mode, int search_limit, BlockDXT1 * output);
    void compress_dxt1_cluster_fit(const Vector4 input_colors[16], const Vector3 * colors, const float * weights, int count, const Vector3 & color_weights, bool three_color_mode, BlockDXT1 * output);

    // Cluster fit end point selection.
    float compress_dxt1(const Vector4 input_colors[16], const float input_weights[16], const Vector3 & color_weights, bool three_color_mode, bool hq, BlockDXT1 * output);

    // Quick end point selection followed by least squares refinement.
    float compress_dxt1_fast(const Vector4 input_colors[16], const float input_weights[16], const Vector3 & color_weights, BlockDXT1 * output);

    // @@ Change these interfaces to take a pitch argument instead of assuming (4*4), just like CMP_Core.
    void compress_dxt1_fast2(const unsigned char input_colors[16*4], BlockDXT1 * output);
    void compress_dxt1_fast_geld(const unsigned char input_colors[16 * 4], BlockDXT1 * output);

    float evaluate_dxt1_error(const unsigned char rgba_block[16 * 4], const BlockDXT1 * block, int decoder = 0);

}
