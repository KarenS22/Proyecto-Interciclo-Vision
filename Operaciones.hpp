#ifndef OPERACIONES_HPP
#define OPERACIONES_HPP

#include <itkImage.h>
#include <opencv2/opencv.hpp>
#include "Tipos.hpp" 


// ============================================================================
// FUNCIONES ITK/OPENCV
// ============================================================================

/**
 * Extrae un slice de una imagen 3D ITK y lo convierte a Mat de OpenCV
 * @param image3D Puntero a la imagen 3D de ITK
 * @param sliceNumber Número del slice a extraer
 * @return Mat normalizada (8 bits, 0-255) para visualización
 */
cv::Mat itkSliceToMat(InputImageType::Pointer image3D, int sliceNumber);



/**
 * Segmenta el corazón en una imagen CT usando valores HU
 * @param rawHU Imagen con valores Hounsfield Units (16 bits con signo)
 * @param maskPulmones Máscara binaria de pulmones (0 = pulmón, 255 = fuera)
 * @return Máscara binaria con el corazón segmentado
 */
cv::Mat segmentarCorazon(const cv::Mat& input, const cv::Mat& maskPulmones);



/**
 * Segmenta los huesos en una imagen CT
 * @param input Imagen en escala de grises (8 bits)
 * @return Máscara binaria con los huesos segmentados
 */
cv::Mat segmentarHuesos(cv::Mat input);


cv::Mat getHeartMask(const cv::Mat& not_air_mask, const cv::Mat& lungs_mask);

cv::Mat segmentarTejidosBlandos(cv::Mat input);

#endif // OPERACIONES_HPP


