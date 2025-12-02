#ifndef HUESOS_HPP
#define HUESOS_HPP

#include <itkImage.h>
#include <opencv2/opencv.hpp>
#include "Tipos.hpp"


/**
 * Segmenta los huesos en una imagen CT
 * @param input Imagen en escala de grises (8 bits)
 * @return Máscara binaria con los huesos segmentados
 */
cv::Mat mostrarHuesosConSliders(cv::Mat input);

#endif // HUESOS_HPP

