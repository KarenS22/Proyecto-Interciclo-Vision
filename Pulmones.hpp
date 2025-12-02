#ifndef PULMONES_HPP
#define PULMONES_HPP

#include <itkImage.h>
#include <opencv2/opencv.hpp>
#include "Tipos.hpp" 


/**
 * Segmenta los pulmones en una imagen CT
 * @param input Imagen en escala de grises (8 bits)
 * @return Máscara binaria con los pulmones segmentados
 */
cv::Mat mostrarPulmonesConSliders(cv::Mat input);

#endif // PULMONES_HPP