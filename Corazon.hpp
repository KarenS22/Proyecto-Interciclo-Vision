#ifndef CORAZON_HPP
#define CORAZON_HPP

#include <itkImage.h>
#include <opencv2/opencv.hpp>
#include "Tipos.hpp"

/**
 * Segmenta el corazón en una imagen CT usando la máscara de pulmones
 * @param input Imagen en escala de grises (8 bits)
 * @return Máscara binaria con el corazón segmentado
 */
cv::Mat mostrarCorazonConSliders(const cv::Mat& input);

#endif // CORAZON_HPP